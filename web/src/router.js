import Vue from 'vue';
import Router from 'vue-router';
import MainContent from './MainContent.vue'
import Admin from './Admin.vue'
Vue.use(Router)

export default new Router({
    routes: [
      {
        path: '/',
        name: 'MainContent',
        component: MainContent
      },
      {
        path: '/admin',
        name: 'Admin',
        component: Admin
      }      
    ]
  });