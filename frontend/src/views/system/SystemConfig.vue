<script setup>
import { ref, onMounted } from 'vue';
import { Modal } from 'usemodal-vue3';

import StatusMessage from '../../components/StatusMessage.vue';
import router from '../../router/index.js';

import { getSystemNetwork, setSystemNetwork } from '../../js/restApi';

const rebootModalVisible = ref(false);
const formEnabled = ref(false);
const hostname = ref('');
const ssid = ref('');
const password = ref('');
const showPassword = ref(false);
const statusMsg = ref('');
const statusMsgIsError = ref(false);

onMounted(() => {
  getSystemNetwork()
    .then((config) => {
      if ('hostname' in config) {
        hostname.value = config.hostname;
      }
      if ('ssid' in config) {
        ssid.value = config.ssid;
      }
      if ('password' in config) {
        password.value = config.password;
      }
      formEnabled.value = true;
    })
    .catch((error) => {
      console.log(error);
      statusMsg.value = `Configuration getting failed with ${error}`;
      statusMsgIsError.value = true;
    });
});

function submit () {
  statusMsg.value = '';
  statusMsgIsError.value = false;
  setSystemNetwork(hostname.value, ssid.value, password.value)
    .then((config) => {
      if ('hostname' in config) {
        hostname.value = config.hostname;
      }
      if ('ssid' in config) {
        ssid.value = config.ssid;
      }
      if ('password' in config) {
        password.value = config.password;
      }
      statusMsg.value = 'Configuration updated successfully';
      statusMsgIsError.value = false;
      rebootModalVisible.value = true;
    })
    .catch((error) => {
      console.log(error);
      statusMsg.value = `Configuration updating failed with ${error}`;
      statusMsgIsError.value = true;
    });
}

function toggleShow () {
  showPassword.value = !showPassword.value;
}

function rebootOk () {
  router.push({ path: 'reboot' });
}
</script>

<template>
  <Modal
    v-model:visible="rebootModalVisible"
    title="Update Finished"
    :ok-button="{ onclick: rebootOk }"
  >
    <div>
      <a>Configuration changes will only be effective after a reboot.<br><br></a>
      <a>Perform a reboot?</a>
    </div>
  </Modal>
  <div class="row">
    <div class="container">
      <br>
      <h1>System Configuration</h1>
      <form @submit.prevent="submit">
        <h3>Network</h3>
        <div class="form-group">
          <label for="hostname">Hostname</label>
          <input
            id="hostname"
            v-model="hostname"
            type="text"
            minlength="1"
            maxlength="32"
            class="form-control"
            :disabled="!formEnabled"
          >
        </div>
        <h3>Wi-Fi</h3>
        <div class="form-group">
          <label for="ssid">SSID</label>
          <input
            id="ssid"
            v-model="ssid"
            type="text"
            minlength="1"
            maxlength="32"
            class="form-control"
            :disabled="!formEnabled"
          >
        </div>
        <div class="form-group">
          <label for="password">Password</label>
          <div
            v-if="showPassword"
            id="show_password"
            class="input-group"
          >
            <input
              id="password"
              v-model="password"
              type="text"
              minlength="1"
              maxlength="64"
              class="form-control"
              :disabled="!formEnabled"
            >
            <div class="input-group-append">
              <button
                class="btn btn-outline-secondary"
                type="button"
                @click="toggleShow"
              >
                <font-awesome-icon icon="fa-solid fa-eye-slash" />
              </button>
            </div>
          </div>
          <div
            v-else
            id="hide_password"
            class="input-group"
          >
            <input
              id="password"
              v-model="password"
              type="password"
              minlength="1"
              maxlength="64"
              class="form-control"
              :disabled="!formEnabled"
            >
            <div class="input-group-append">
              <button
                class="btn btn-outline-secondary"
                type="button"
                @click="toggleShow"
              >
                <font-awesome-icon icon="fa-solid fa-eye" />
              </button>
            </div>
          </div>
        </div>
        <br>
        <div class="form-group">
          <button
            class="btn btn-primary"
            :disabled="!formEnabled"
          >
            Save
          </button>
        </div>
      </form>
      <StatusMessage
        :is-error="statusMsgIsError"
        :message="statusMsg"
      />
    </div>
  </div>
</template>
