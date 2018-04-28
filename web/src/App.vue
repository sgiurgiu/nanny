<template>
  <div id="app">      
      <nanny-nav/>
      <main-content/>
      
    <footer class="container">
      <p>&copy; Sergiu Giurgiu 2018</p>
    </footer>      
  </div>
</template>

<script>
import NannyNav from './NannyNav.vue'
import MainContent from './MainContent.vue'
import Other from './Other.vue'

export default {
  name: 'app'  ,
  components: {
    MainContent,
    NannyNav,
    Other
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
