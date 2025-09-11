<script setup>
import { ref } from 'vue';
import { EthMcm } from '../../js/ethMcm.js';

import ProgressBar from '../../components/ProgressBar.vue';
import StatusMessage from '../../components/StatusMessage.vue';

const OPERATION_PROGRAM = 'Program';
const OPERATION_VERIFY = 'Verify';

const products = ref([
  { projId: 4609, fullName: 'MLX81113xAA', EepromVerif: false, flashCsProg: false },
  { projId: 4611, fullName: 'MLX81113xAB', EepromVerif: false, flashCsProg: false },
  { projId: 6145, fullName: 'MLX81118xAA', EepromVerif: false, flashCsProg: false },
  { projId: 6146, fullName: 'MLX81118xAB', EepromVerif: true, flashCsProg: true },
  { projId: 5633, fullName: 'MLX81160-xLW-AMx-001', EepromVerif: false, flashCsProg: false },
  { projId: 1300, fullName: 'MLX81330-xDC-BMx-002', EepromVerif: false, flashCsProg: false },
  { projId: 1301, fullName: 'MLX81330-xLW-BMx-102', EepromVerif: false, flashCsProg: false },
  { projId: 1302, fullName: 'MLX81330-xDC-BMx-102', EepromVerif: false, flashCsProg: false },
  { projId: 1303, fullName: 'MLX81330-xDC-BMx-202', EepromVerif: false, flashCsProg: false },
  { projId: 1304, fullName: 'MLX81330-xDC-BMx-302', EepromVerif: false, flashCsProg: false },
  { projId: 1305, fullName: 'MLX81330-xDC-BMx-402', EepromVerif: false, flashCsProg: false },
  { projId: 1807, fullName: 'MLX81332-xLW-BMx-202', EepromVerif: false, flashCsProg: false },
  { projId: 1808, fullName: 'MLX81332-xDC-BMx-002', EepromVerif: false, flashCsProg: false },
  { projId: 1809, fullName: 'MLX81332-xDC-BMx-102', EepromVerif: false, flashCsProg: false },
  { projId: 1810, fullName: 'MLX81332-xDC-BMx-202', EepromVerif: false, flashCsProg: false },
  { projId: 1811, fullName: 'MLX81332-xDC-BMx-302', EepromVerif: false, flashCsProg: false },
  { projId: 2305, fullName: 'MLX81334-xLW-AMx-001', EepromVerif: false, flashCsProg: false },
  { projId: 9475, fullName: 'MLX81339-xDC-AMx-202', EepromVerif: true, flashCsProg: false },
  { projId: 9476, fullName: 'MLX81339-xLW-AMx-202', EepromVerif: true, flashCsProg: false },
  { projId: 9477, fullName: 'MLX81339-xDC-BMx-202', EepromVerif: true, flashCsProg: false },
  { projId: 9478, fullName: 'MLX81339-xLW-BMx-202', EepromVerif: true, flashCsProg: false },
  { projId: 2565, fullName: 'MLX81340-xLW-BMx-003', EepromVerif: false, flashCsProg: false },
  { projId: 2566, fullName: 'MLX81340-xLW-BMx-103', EepromVerif: false, flashCsProg: false },
  { projId: 3075, fullName: 'MLX81344-xLW-BMx-003', EepromVerif: false, flashCsProg: false },
  { projId: 3076, fullName: 'MLX81344-xLW-BMx-103', EepromVerif: false, flashCsProg: false },
  { projId: 3589, fullName: 'MLX81346-xLW-BMx-003', EepromVerif: false, flashCsProg: false },
  { projId: 3590, fullName: 'MLX81346-xPF-BMx-003', EepromVerif: false, flashCsProg: false },
  { projId: 9729, fullName: 'MLX81350-xDC-AMx-001', EepromVerif: true, flashCsProg: false },
  { projId: 9730, fullName: 'MLX81350-xDC-AMx-101', EepromVerif: true, flashCsProg: false },
  { projId: 9731, fullName: 'MLX81350-xDC-AMx-201', EepromVerif: true, flashCsProg: false },
  { projId: 9732, fullName: 'MLX81350-xLW-AMx-101', EepromVerif: true, flashCsProg: false },
  { projId: 9733, fullName: 'MLX81350-xLW-AMx-900', EepromVerif: true, flashCsProg: false },
  { projId: 9734, fullName: 'MLX81350-xDC-BMx-002', EepromVerif: true, flashCsProg: false },
  { projId: 9735, fullName: 'MLX81350-xDC-BMx-102', EepromVerif: true, flashCsProg: false },
  { projId: 9736, fullName: 'MLX81350-xDC-BMx-202', EepromVerif: true, flashCsProg: false },
  { projId: 9737, fullName: 'MLX81350-xLW-BMx-102', EepromVerif: true, flashCsProg: false },
  { projId: 9738, fullName: 'MLX81350-xLW-BMx-902', EepromVerif: true, flashCsProg: false },
  { projId: 9985, fullName: 'MLX81352-xDC-AMx-000', EepromVerif: true, flashCsProg: false },
  { projId: 9986, fullName: 'MLX81352-xDC-AMx-100', EepromVerif: true, flashCsProg: false },
  { projId: 9987, fullName: 'MLX81352-xDC-AMx-200', EepromVerif: true, flashCsProg: false },
  { projId: 9988, fullName: 'MLX81352-xLW-AMx-100', EepromVerif: true, flashCsProg: false },
  { projId: 10241, fullName: 'MLX81354-xDC-AMx-000', EepromVerif: true, flashCsProg: false },
  { projId: 10242, fullName: 'MLX81354-xDC-AMx-100', EepromVerif: true, flashCsProg: false },
  { projId: 10243, fullName: 'MLX81354-xDC-AMx-200', EepromVerif: true, flashCsProg: false },
  { projId: 10244, fullName: 'MLX81354-xLW-AMx-100', EepromVerif: true, flashCsProg: false },
  { projId: 3329, fullName: 'MLX91230-xDC-AAx-xxx', EepromVerif: false, flashCsProg: false },
  { projId: 3330, fullName: 'MLX91230-xDC-BAx-xxx', EepromVerif: true, flashCsProg: false },
  { projId: 3331, fullName: 'MLX9123x-KDC-BBA-000', EepromVerif: true, flashCsProg: true },
  { projId: 3332, fullName: 'MLX91230KDC-BBA-100', EepromVerif: true, flashCsProg: true },
  { projId: 3333, fullName: 'MLX91230KDC-BBC-000', EepromVerif: true, flashCsProg: true },
  { projId: 3334, fullName: 'MLX91230KDC-BBC-100', EepromVerif: true, flashCsProg: true },
  { projId: 3335, fullName: 'MLX91231KDC-BBC-000', EepromVerif: true, flashCsProg: true },
  { projId: 3336, fullName: 'MLX91230KDC-BBC-200', EepromVerif: true, flashCsProg: true },
  { projId: 3337, fullName: 'MLX91231KGO-BBC-000', EepromVerif: true, flashCsProg: true },
]);
const manualPower = ref(false);
const enableBroadcast = ref(false);
const selProduct = ref(null);
const bitRate = ref(300000);
const flashFile = ref(null);
const flashCsFile = ref(null);
const eepromFile = ref(null);
const errorMsg = ref('');
const isErrorMsg = ref(false);
const progbarProgress = ref(0);
const progbarIsAnimated = ref(false);

let master = null;

function setErrorMessage (msg, isError = true) {
  errorMsg.value = msg;
  isErrorMsg.value = isError;
}

function onFileChange (type, e) {
  const files = e.target.files || e.dataTransfer.files;
  if (!files.length) return;

  const file = files[0];
  if (file && !file.name.endsWith('.hex')) {
    setErrorMessage('Only .hex files are supported');
    return;
  }

  switch (type) {
    case 'flash':
      flashFile.value = file;
      break;
    case 'flashCs':
      flashCsFile.value = file;
      break;
    case 'eeprom':
      eepromFile.value = file;
      break;
  }
}

function connectMaster () {
  if (master) return Promise.resolve(master);

  setErrorMessage('Connecting...', false);

  master = new EthMcm();
  master.on('disconnect', function () {
    master = null;
  });

  return master.connect(location.hostname)
    .catch((error) => {
      master = null;
      return Promise.reject(error);
    });
}

function getFileContent (file) {
  return new Promise(function (resolve, reject) {
    const fileReader = new FileReader();
    fileReader.addEventListener('load', () => {
      resolve(fileReader.result);
    });
    fileReader.addEventListener('error', () => {
      reject(new Error(`Failed reading file '${file.name}' with '${fileReader.error}'`));
    });
    fileReader.readAsBinaryString(file);
  });
}

function program (operation, memory) {
  if (enableBroadcast.value && selProduct.value == null) {
    setErrorMessage('Please select a product first');
    return;
  }

  let file;
  progbarProgress.value = 0;
  progbarIsAnimated.value = true;
  setErrorMessage('', false);

  switch (memory) {
    case 'Flash':
      file = flashFile.value;
      break;
    case 'Flash_CS':
      file = flashCsFile.value;
      break;
    case 'EEPROM':
      file = eepromFile.value;
      break;
  }

  if (file === null) {
    progbarProgress.value = 0;
    progbarIsAnimated.value = false;
    setErrorMessage('Select a hex file first');
    return;
  }
  doAction(operation, memory, file)
    .then(() => {
      progbarProgress.value = 100;
      progbarIsAnimated.value = false;
      setErrorMessage(`${operation} ${memory} finished successfully`, false);
    })
    .catch((error) => {
      progbarProgress.value = 0;
      progbarIsAnimated.value = false;
      setErrorMessage(error.message);
    });
}

function doAction (operation, memory, file) {
  if (master === null || typeof (master) === 'undefined') {
    return connectMaster()
      .then(() => {
        return doAction(operation, memory, file);
      });
  } else {
    progbarProgress.value = 10;
    setErrorMessage('Connection opened...', false);
    return getFileContent(file)
      .then((hexfile) => {
        progbarProgress.value = 15;
        setErrorMessage(`File successfully processed, ${memory} operation in progress...`, false);
        const params = {
          hexfile,
          memory: memory.toLowerCase(),
          manpow: manualPower.value,
          bitrate: bitRate.value,
          project: enableBroadcast.value ? selProduct.value.projId : 0
        };
        return master.sendTask('bootloader', operation.toLowerCase(), params);
      });
  }
}

function isProductSelected () {
  return selProduct.value != null;
}

function isFlashEnabled () {
  return flashFile.value && (!enableBroadcast.value || isProductSelected());
}

function isEepromProgrammingEnabled () {
  return eepromFile.value && (!enableBroadcast.value || isProductSelected());
}

function isEepromVerificationEnabled () {
  return eepromFile.value && (!enableBroadcast.value || (isProductSelected() && selProduct.value.EepromVerif));
}

function isFlashCsEnabled () {
  return !enableBroadcast.value || (isProductSelected() && selProduct.value.flashCsProg);
}
</script>

<template>
  <div class="row">
    <div class="container">
      <br>
      <h1>One2one PPM Programmer</h1>
      <h2>Configuration</h2>
      <form @submit.prevent>
        <div
          id="manualpower"
          class="form-check"
        >
          <input
            id="checkmanualpower"
            v-model="manualPower"
            class="form-check-input"
            type="checkbox"
            title="Enable manual power cycling for the bootloader entry."
          >
          <label
            class="form-check-label"
            for="checkmanualpower"
          >
            Manual power cycling
          </label>
        </div>
        <div
          id="broadcast"
          class="form-check"
        >
          <input
            id="checkEnablebroadcast"
            v-model="enableBroadcast"
            class="form-check-input"
            type="checkbox"
            title="Enable broadcast programming, in this mode there is no chip feedback or verification."
          >
          <label
            class="form-check-label"
            for="checkEnablebroadcast"
          >
            Broadcast programming
          </label>
        </div>
        <div
          v-if="enableBroadcast"
          id="product"
        >
          <label for="selectproduct">Product</label>
          <select
            id="selectproduct"
            v-model="selProduct"
            class="form-select"
            title="Choose the specific product from the list."
          >
            <option
              disabled
              value="null"
            >
              Please select product
            </option>
            <option
              v-for="product in products"
              :key="product.projId"
              :value="product"
            >
              {{ product.fullName }}
            </option>
          </select>
        </div>
        <div id="bitrate">
          <label for="selectbitrate">Bit rate</label>
          <select
            id="selectbitrate"
            v-model.number="bitRate"
            class="form-select"
            title="Choose a bit rate from the available options."
          >
            <option
              selected
              value="300000"
            >
              300 kbps
            </option>
            <option value="250000">
              250 kbps
            </option>
            <option value="200000">
              200 kbps
            </option>
            <option value="150000">
              150 kbps
            </option>
            <option value="125000">
              125 kbps
            </option>
            <option value="100000">
              100 kbps
            </option>
            <option value="75000">
              75 kbps
            </option>
            <option value="50000">
              50 kbps
            </option>
            <option value="25000">
              25 kbps
            </option>
          </select>
        </div>
        <div id="flash">
          <br>
          <h2>Flash</h2>
          <input
            type="file"
            class="mlx-file"
            accept=".hex"
            @change="onFileChange('flash', $event)"
            title="Select the .hex file to program the flash memory."
          >
          <button
            class="btn btn-primary"
            :disabled="!isFlashEnabled()"
            @click="program(OPERATION_PROGRAM, 'Flash')"
            title="Click to start programming the flash memory."
          >
            Program
          </button>
          <button
            class="btn btn-primary"
            :disabled="!isFlashEnabled()"
            @click="program(OPERATION_VERIFY, 'Flash')"
            title="Click to verify the flash memory against the selected file."
          >
            Verify
          </button>
        </div>
        <div
          v-if="isFlashCsEnabled()"
          id="flash_cs"
        >
          <br>
          <h2>Flash CS</h2>
          <input
            type="file"
            class="mlx-file"
            accept=".hex"
            @change="onFileChange('flashCs', $event)"
            title="Select the .hex file to program the flash CS memory."
          >
          <button
            class="btn btn-primary"
            :disabled="!flashCsFile"
            @click="program(OPERATION_PROGRAM, 'Flash_CS')"
            title="Click to start programming the flash CS memory."
          >
            Program
          </button>
          <button
            class="btn btn-primary"
            :disabled="!flashCsFile"
            @click="program(OPERATION_VERIFY, 'Flash_CS')"
            title="Click to verify the flash CS memory against the selected file."
          >
            Verify
          </button>
        </div>
        <div id="eeprom">
          <br>
          <h2>EEPROM</h2>
          <input
            type="file"
            class="mlx-file"
            accept=".hex"
            @change="onFileChange('eeprom', $event)"
            title="Select the .hex file to program the EEPROM memory."
          >
          <button
            class="btn btn-primary"
            :disabled="!isEepromProgrammingEnabled()"
            @click="program(OPERATION_PROGRAM, 'EEPROM')"
            title="Click to start programming the EEPROM memory."
          >
            Program
          </button>
          <button
            class="btn btn-primary"
            :disabled="!isEepromVerificationEnabled()"
            @click="program(OPERATION_VERIFY, 'EEPROM')"
            title="Click to verify the EEPROM memory against the selected file."
          >
            Verify
          </button>
        </div>
      </form>
      <br>
      <ProgressBar
        maximum="100"
        :value="progbarProgress"
        precision="0"
        :is-animated="progbarIsAnimated"
      />
      <StatusMessage
        :is-error="isErrorMsg"
        :message="errorMsg"
      />
    </div>
  </div>
</template>

<style scoped>
  .mlx-file {
    width: 100%;
    padding: 5px;
    margin: 5px 0;
  }
</style>
