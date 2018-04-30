<template>
  <div id="app">      
      <nanny-nav/> 
      <router-view></router-view>
    <footer class="container">
      <p>&copy; Sergiu Giurgiu 2018</p>
    </footer>      
  </div>
</template>

<script>
import NannyNav from './NannyNav.vue'

export default {
  name: 'app'  ,
  components: {
    NannyNav
  }
}
</script>

<style>
#app {
  font-family: 'Avenir', Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
}

</style>
