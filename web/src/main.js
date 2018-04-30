import Vue from 'vue';
import BootstrapVue from "bootstrap-vue";
import App from './App.vue';
import "bootstrap/dist/css/bootstrap.min.css";
import "bootstrap-vue/dist/bootstrap-vue.css";
import VueResource from 'vue-resource';
import router from './router';



Vue.use(BootstrapVue)
Vue.use(VueResource);
Vue.config.productionTip = false


new Vue({
  el: '#app',
  render: h => h(App),
  router
})
