import { EthernetTransport } from '../js/ethernetTransport.js';

/** Ethernet variant of the Melexis Compact Master (MCM). */
export class EthMcm extends EthernetTransport {
  /** Send a task.
   *
   * @param {string} endpoint - Endpoint for the task.
   * @param {string} command - Command to execute.
   * @param {Object|null} params - Optional parameters for the command.
   * @returns {Promise<any>} Resolves with the task result.
   */
  sendTask (endpoint, command, params = null) {
    return this.sendCommand(endpoint, command, params);
  }

  /** Bootload a HEX file into the device.
   *
   * @param {string} hexfile - HEX file content.
   * @param {string} operation - Bootloader operation.
   * @param {string} memory - Target memory type.
   * @param {boolean} manualPower - Whether manual power is applied.
   * @param {number} bitRate - LIN bus bitrate.
   * @param {boolean} fullDuplex - Full duplex mode.
   * @param {number} txPin - TX pin identifier.
   * @param {Array<number>} flashKeys - Optional flash key sequence.
   * @returns {Promise<any>} Resolves with bootload response.
   */
  bootload (hexfile, operation, memory, manualPower, bitRate, fullDuplex, txPin, flashKeys) {
    if (this.mode !== null) {
      this.disableBareUartMode();
    }
    const params = {
      memory,
      hexfile,
      one2many: true,
      manpow: manualPower,
      bitrate: bitRate,
      fullduplex: fullDuplex,
      txpin: txPin,
      flashkeys: flashKeys
    };
    this.pausePing = true;
    this.mode = 'bootloader';
    return this.sendTask('bootloader', operation, params)
      .then((response) => {
        this.mode = null;
        this.pausePing = false;
        return Promise.resolve(response);
      })
      .catch((error) => {
        this.mode = null;
        this.pausePing = false;
        return Promise.reject(error);
      });
  }

  /** Disable power to slave devices.
   *
   * @returns {Promise<any>}
   */
  disableSlavePower () {
    return this.sendTask('power_out', 'control', { switch_enable: false });
  }

  /** Enable power to slave devices.
   *
   * @returns {Promise<any>}
   */
  enableSlavePower () {
    return this.sendTask('power_out', 'control', { switch_enable: true });
  }

  /** Check if slave power is enabled.
   *
   * @returns {Promise<boolean>}
   */
  isSlavePowerEnabled () {
    return this.sendTask('power_out', 'status')
      .then((response) => {
        return Promise.resolve(response.switch_enabled);
      });
  }
}
