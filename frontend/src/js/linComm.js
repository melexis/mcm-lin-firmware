export function lIfcWakeUp (master) {
  const params = { pulse_time: 200 };
  return master.sendTask('lin', 'l_ifc_wake_up', params);
}

export function lM2s (master, baudrate, enhancedCrc, frameid, payload) {
  const params = {
    datalength: payload.length,
    m2s: true,
    baudrate,
    enhanced_crc: enhancedCrc,
    frameid,
    payload
  };
  return master.sendTask('lin', 'handle_message_on_bus', params);
}

export function lS2m (master, baudrate, enhancedCrc, frameid, datalength) {
  const params = {
    datalength,
    m2s: false,
    baudrate,
    enhanced_crc: enhancedCrc,
    frameid
  };
  return master.sendTask('lin', 'handle_message_on_bus', params)
    .then((result) => {
      return result.data;
    });
}

export function ldDiagnostic (master, nad, baudrate, sid, payload) {
  payload.splice(0, 0, sid);
  const data = {
    protocol: 'lin',
    function: 'ld_diagnostic',
    args: {
      nad,
      baudrate,
      payload
    }
  };
  return master.sendTask(data)
    .then((result) => {
      const data = result.data;
      if (data[0] === 0x7F) {
        // slave reported an error
        return Promise.reject(new Error(`Error 0x${data[2].toString(16)} was reported by ` +
                                        `the device for SID 0x${data[1].toString(16)}`));
      } else if (data[0] !== ((sid + 0x40) & 0xFF)) {
        return Promise.reject(new Error(`An incorrect RSID was received (0x${data[0].toString(16)})`));
      } else {
        return Promise.resolve(data.slice(1));
      }
    });
}

export function ldSendMessage (master, nad, baudrate, sid, payload) {
  payload.splice(0, 0, sid);
  const data = {
    protocol: 'lin',
    function: 'ld_send_message',
    args: {
      nad,
      baudrate,
      payload
    }
  };
  return master.sendTask(data);
}

export function ldReceiveMessage (master, nad, baudrate, sid) {
  const data = {
    protocol: 'lin',
    function: 'ld_receive_message',
    args: {
      nad,
      baudrate
    }
  };
  return master.sendTask(data)
    .then((result) => {
      const data = result.data;
      if (data[0] === 0x7F) {
        // slave reported an error
        return Promise.reject(new Error(`Error 0x${data[2].toString(16)} was reported by ` +
                                        `the device for SID 0x${data[1].toString(16)}`));
      } else if (data[0] !== ((sid + 0x40) & 0xFF)) {
        return Promise.reject(new Error(`An incorrect RSID was received (0x${data[0].toString(16)})`));
      } else {
        return Promise.resolve(data.slice(1));
      }
    });
}

export function readById (master, nad, baudrate, identifier, supplierId = 0x7FFF, functionId = 0xFFFF) {
  return ldDiagnostic(
    master,
    nad,
    baudrate,
    0xB2,
    [identifier, supplierId & 0xFF, (supplierId >> 8) & 0xFF, functionId & 0xFF, (functionId >> 8) & 0xFF]
  );
}
