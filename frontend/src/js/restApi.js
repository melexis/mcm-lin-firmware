import axios from 'axios';

export const resetReasons = {
  0: 'Reset reason can not be determined',
  1: 'Reset due to power-on event',
  2: 'Reset by external pin (not applicable for ESP32)',
  3: 'Software reset via esp_restart',
  4: 'Software reset due to exception/panic',
  5: 'Reset (software or hardware) due to interrupt watchdog',
  6: 'Reset due to task watchdog',
  7: 'Reset due to other watchdogs',
  8: 'Reset after exiting deep sleep mode',
  9: 'Brownout reset (software or hardware)',
  10: 'Reset over SDIO',
  11: 'Reset by USB peripheral',
  12: 'Reset by JTAG'
};

function axiosErrorToString (error) {
  if (error.response) {
    // The request was made and the server responded with a status code
    // that falls out of the range of 2xx
    console.log(error.response.data);
    console.log(error.response.status);
    console.log(error.response.headers);
    return error.response.statusText;
  } else if (error.request) {
    // The request was made but no response was received
    // `error.request` is an instance of XMLHttpRequest in the browser and an instance of
    // http.ClientRequest in node.js
    return error.request;
  } else {
    // Something happened in setting up the request that triggered an Error
    return `Error ${error.message}`;
  }
}

export const systemReboot = function () {
  return new Promise((resolve, reject) => {
    axios
      .put('/api/v1/system/reboot')
      .then(resolve)
      .catch(function (error) {
        reject(new Error(axiosErrorToString(error)));
      });
  });
};

export const systemIdentify = function () {
  return new Promise((resolve, reject) => {
    axios
      .put('/api/v1/system/identify')
      .then(resolve)
      .catch(function (error) {
        reject(new Error(axiosErrorToString(error)));
      });
  });
};

export const getSystemInfo = function () {
  return new Promise((resolve, reject) => {
    axios
      .get('/api/v1')
      .then((response) => {
        resolve(response.data);
      })
      .catch(function (error) {
        reject(new Error(axiosErrorToString(error)));
      });
  });
};

export const getSystemNetwork = function () {
  return new Promise((resolve, reject) => {
    axios
      .get('/api/v1/system/wifi')
      .then((response) => {
        resolve(response.data);
      })
      .catch(function (error) {
        reject(new Error(axiosErrorToString(error)));
      });
  });
};

export const setSystemNetwork = function (hostname, ssid, password) {
  return new Promise((resolve, reject) => {
    axios
      .put('/api/v1/system', { hostname, ssid, password })
      .then((response) => {
        resolve(response.data);
      })
      .catch(function (error) {
        reject(new Error(axiosErrorToString(error)));
      });
  });
};
