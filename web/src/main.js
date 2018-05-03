import Vue from 'vue';
import BootstrapVue from "bootstrap-vue";
import App from './App.vue';
import "bootstrap/dist/css/bootstrap.min.css";
import "bootstrap-vue/dist/bootstrap-vue.css";
import VueResource from 'vue-resource';
import router from './router';
import VueLocalStorage from 'vue-localstorage';
import 'font-awesome/scss/font-awesome.scss';
import VueMoment from 'vue-moment';

Vue.use(BootstrapVue)
Vue.use(VueResource);
Vue.use(VueMoment);
Vue.use(VueLocalStorage, {
  name: 'ls',
  bind: true //created computed members from your variable declarations
});

Vue.config.productionTip = false
Vue.http.interceptors.push(function(request) {
  // modify headers
  const token = this.$ls.get('login_token');
  if(token != null) {
    request.headers.set('Authorization', 'Bearer '+token);
  }
  request.url+=request.url.indexOf('?') === -1 ? '?' : '&';
  request.url += 'cacheBuster=' + new Date().getTime();  
});

new Vue({
  el: '#app',
  render: h => h(App),
  router
  
})
