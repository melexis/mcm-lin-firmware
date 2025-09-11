/** Ethernet-based transport for communicating with MCM devices. */
export class EthernetTransport {
  /** Class constructor. */
  constructor () {
    this.state = {
      socket: null,
      discovering: false,
      clientTaskQueuePos: 0,
      clientTaskQueue: {},
      keepAliveTimer: null,
      isAlive: false,
      pausePing: false,
      events: {
        error: null,
        disconnect: null
      }
    };
  }

  /** Pause or resume sending ping messages.
   *
   * @type {boolean}
   */
  get pausePing () {
    return this.state.pausePing;
  }

  set pausePing (value) {
    this.state.pausePing = value;
  }

  /* Register an event handler to a specific event.
   *
   * @param {string} eventName - name of the event to register a handler for.
   * @param {function} eventHandler - handler to be registered to the event.
   */
  on (eventName, eventHandler) {
    this.state.events[eventName] = eventHandler;
  }

  /** Test whether the connection with the hardware is established.
   *
   * @returns {boolean} true when the connection with the hardware is established.
   */
  isConnected () {
    if (this.state.socket === null) {
      return false;
    }
    return this.state.socket.readyState === this.state.socket.OPEN;
  }

  /** Connect to the hardware.
   *
   * @returns {Promise<string>} Resolves with the device type name of the connected HW.
   */
  connect (hostname) {
    if (this.isConnected()) {
      return Promise.resolve();
    }
    return new Promise((resolve, reject) => {
      this.state.discovering = true;
      let protocol = 'wss';
      if (window.location.protocol === 'http:') {
        protocol = 'ws';
      }
      this.state.socket = new window.WebSocket(`${protocol}://${hostname}/ws/v1`);

      this.state.socket.addEventListener('open', () => {
        resolve();
      });

      this.state.socket.addEventListener('message', (event) => {
        let data;
        try {
          data = JSON.parse(event.data);
        } catch (er) {
          console.log(`Parse error: ${event.data}`);
        }
        if (typeof (data.__pong__) !== 'undefined' && data.__pong__ === true) {
          this.state.isAlive = true;
        } else if (typeof (data.__ping__) !== 'undefined' && data.__ping__ === true) {
          this.state.socket.send(JSON.stringify({ __pong__: true }));
          this.state.isAlive = true;
        } else {
          this.onMessage(data);
        }
      });

      this.state.socket.addEventListener('close', (event) => {
        if (typeof (this.state.events.disconnect) === 'function') {
          this.state.events.disconnect(event.reason);
        }
        this.state.socket = null;
      });

      this.state.socket.addEventListener('error', (event) => {
        console.log(`WebSocket error: ${event.error}`);
        reject(new Error(`Connection to MCM failed: ${event.message}`));
      });
    });
  }

  /** Disconnect from the hardware.
   *
   * @param {string} message - Optional human-readable reason.
   * @returns {Promise<void>} Resolves when the disconnect operation completes.
   */
  disconnect (message = '') {
    if (this.state.keepAliveTimer !== null) {
      clearInterval(this.state.keepAliveTimer);
      this.state.keepAliveTimer = null;
    }
    if (this.state.socket !== null) {
      return new Promise((resolve) => {
        const socket = this.state.socket;
        const onClose = () => {
          socket.removeEventListener('close', onClose);
          this.state.socket = null;
          this.state.apiRev = null;
          resolve();
        };
        socket.addEventListener('close', onClose);
        socket.close(1000, message);
      });
    }
    return Promise.resolve();
  }

  /** Request info from the device.
   *
   * @returns {Promise<Object>} Resolves with info payload
   */
  reqInfo () {
    return this.sendMessage('info');
  }

  /** Send a command.
   *
   * @param {string} endpoint - Endpoint name
   * @param {string} command - Command name
   * @param {Object|null} params - Optional parameters
   * @returns {Promise<any>} Resolves with the response
   */
  sendCommand (endpoint, command, params = null) {
    let payload;
    if (params === null) {
      payload = { endpoint, command };
    } else {
      payload = { endpoint, command, params };
    }
    return this.sendMessage('command', payload);
  }

  /** Send a generic message.
   *
   * @param {string} type - Message type ('command' or 'info')
   * @param {Object|null} payload - Optional message payload
   * @returns {Promise<any>} Resolves with the response
   */
  sendMessage (type, payload = null) {
    if (!this.isConnected()) {
      return Promise.reject(new Error('Connection closed'));
    }
    return new Promise((resolve, reject) => {
      this.state.clientTaskQueuePos++;
      this.state.clientTaskQueue['id_' + this.state.clientTaskQueuePos] = { resolve, reject };
      const id = `${this.state.clientTaskQueuePos}`;
      let message;
      if (payload === null) {
        message = { id, type };
      } else {
        message = { id, type, payload };
      }
      try {
        this.state.socket.send(JSON.stringify(message));
      } catch (error) {
        console.error(error);
        reject(new Error('Sending task to MCM failed'));
      }
    });
  }

  /** Handle incoming messages.
   *
   * @param {Object} data - Parsed JSON message
   */
  onMessage (data) {
    if (typeof (data.id) !== 'undefined' && `id_${data.id}` in this.state.clientTaskQueue) {
      const execFuncs = this.state.clientTaskQueue[`id_${data.id}`];
      if (data.type === 'ack') {
        if (typeof (execFuncs.resolve) === 'function') {
          execFuncs.resolve(data.payload);
        }
      } else if (data.type === 'error') {
        if (typeof (execFuncs.reject) === 'function') {
          execFuncs.reject(new Error(data.payload.message));
        }
      }
      delete this.state.clientTaskQueue[`id_${data.id}`];
    } else {
      console.log(`Unhandled task ${data}`);
    }
  }

  /** Send a ping to the connected hardware. */
  sendPing () {
    if (this.isConnected() && !this.state.pausePing) {
      if (!this.state.isAlive) {
        this.disconnect('connection lost');
      } else {
        this.state.socket.send(JSON.stringify({ __ping__: true }));
        this.state.isAlive = false;
      }
    }
  }

  /** Request the connected hardware to identify itself.
   *
   * @returns {Promise<void>} Resolves when the identify request completes.
   */
  identify () { return Promise.resolve(); }

  /** Getter for the connected hardware software version information.
   *
   * @returns {Promise<string>} Resolves with the software version information for the connected HW.
   */
  getVersion () {
    return this.reqInfo()
      .then((response) => {
        return Promise.resolve(response.firmware_version);
      });
  }

  /** Transport object device type name.
   *
   * @returns {Promise<string>} Resolves with the device type name of the connected HW.
   */
  getDeviceType () {
    return this.reqInfo()
      .then((response) => {
        return Promise.resolve(response.model);
      });
  }
}
