import { createRouter, createWebHistory } from 'vue-router';

import HomePage from '@/views/HomePage.vue';
import TermsOfUse from '@/views/TermsOfUse.vue';
import LinCommander from '@/views/lin/LinCommander.vue';
import One2One from '@/views/ppm/One2One.vue';
import SystemDetails from '@/views/system/SystemDetails.vue';
import SystemConfig from '@/views/system/SystemConfig.vue';
import SystemReboot from '@/views/system/SystemReboot.vue';

const routes = [
  {
    path: '/',
    redirect: '/webapp',
  },
  {
    path: '/webapp',
    children: [
      {
        path: '',
        name: 'home',
        component: HomePage,
      },
      {
        path: 'terms-of-use',
        name: 'terms-of-use',
        component: TermsOfUse,
      },
      {
        path: 'lin',
        meta: { requiresMaster: true },
        children: [
          {
            path: '',
            name: 'lin',
            redirect: { name: 'lin-commander' },
          },
          {
            path: 'commander',
            name: 'lin-commander',
            component: LinCommander,
          },
        ],
      },
      {
        path: 'ppm',
        meta: { requiresMaster: true },
        children: [
          {
            path: '',
            name: 'ppm',
            redirect: { name: 'ppm-one2one' },
          },
          {
            path: 'one2one',
            name: 'ppm-one2one',
            component: One2One,
          },
        ],
      },
      {
        path: 'system',
        meta: { requiresMaster: true },
        children: [
          {
            path: '',
            name: 'system',
            redirect: { name: 'system-details' },
          },
          {
            path: 'details',
            name: 'system-details',
            component: SystemDetails,
          },
          {
            path: 'config',
            name: 'system-config',
            component: SystemConfig,
          },
          {
            path: 'reboot',
            name: 'system-reboot',
            component: SystemReboot,
          },
        ],
      },
    ],
  },
];

const router = createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes
});

export default router;
