<template>

 <b-navbar type="dark" variant="dark" fixed="top" toggleable>    
    <b-navbar-brand to="/">Web Nanny</b-navbar-brand>
    <b-navbar-toggle target="nav_dropdown_collapse"></b-navbar-toggle>
    <b-collapse is-nav id="nav_dropdown_collapse">
      <b-navbar-nav :fill="true">
      </b-navbar-nav>
    </b-collapse>
    <b-nav-form>
      <b-button variant="primary" class="my-2 my-sm-0" type="button" v-on:click="adminPage">Admin</b-button>        
      
      <b-modal ref="loginModal" title="Administrator Login" 
          v-on:ok="login" ok-title="Login" :ok-disabled="canWeLogin" v-on:shown="focusUsername">
          <b-form >
             <b-form-group horizontal
                  label="Username:"
                  :label-cols="5"
                  label-for="username" class="pb-2">
                <b-form-input type="text" placeholder="Username" id="username" ref="username"
                        v-model="username" required autofocus></b-form-input>            
              </b-form-group>
             <b-form-group horizontal
                  label="Password:"
                  :label-cols="5"
                  label-for="password">
              <b-form-input type="password" placeholder="Password" id="password"
                      v-model="password" required ></b-form-input>
             </b-form-group>
          </b-form>
     </b-modal>
    </b-nav-form>
  </b-navbar>

</template>

<script>

export default {
  name: 'NannyNav' ,
  data() {
    return {
        username:'',
        password:''
    }
  },
  computed:{
    canWeLogin: function() {      
      return this.username == null || this.username.length <= 0;
    }
  },
  methods:{
    focusUsername: function(event) {
      this.$refs.username.focus();
    },
    adminPage: function(event) {
      this.$http.get('/api/admin').then(result => {this.$router.push('/admin');},error => {this.$refs.loginModal.show();});       
    },
    login: function(event) {
      event.preventDefault();
      this.$http.post("/api/login",{"username":this.username, "password":this.password}).then(result => {
          console.log(JSON.stringify(result.data)); 
          const token = result.data.id;
          this.$ls.set('login_token', token);
          this.$router.push('/admin');
          this.$refs.loginModal.hide();

      }, error => {
          console.error(error);
      });
    }    
  }
}
</script>