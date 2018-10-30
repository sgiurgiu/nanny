<template>
    <div id="admin-content">
      <div class="row">
        <div class="col-2">
          <div class="row">
            <b-card header="<b>Hosts</b>">
              <b-list-group>
                  <b-list-group-item v-for="host in hosts" :key="host.host" href="#" v-bind:class="{'active':(host === currentHost)}"
                    :variant="host_status(host)" v-on:click="displayHostDetails(host)">
                    {{host.host}}
                  </b-list-group-item>
              </b-list-group>
            </b-card>
          </div>
          <div class="row">
            <b-button variant="secondary" class="my-2 my-sm-0" type="button" v-on:click="changePassword">Change Admin Password</b-button>        
          </div>
          <div class="row">  
            <b-button variant="secondary" class="my-2 my-sm-0" type="button" v-on:click="logout">Logout</b-button>        
          </div>  
            <b-modal ref="changePasswordModal" title="Change Password" 
                v-on:ok="changePasswordSubmit" ok-title="Change" :ok-disabled="canWeUpdatePassword" v-on:shown="focusPassword">
                <b-alert variant="error" show="change_password.error != null" dismissible>Cannot change password</b-alert>
                <b-form @submit="changePasswordSubmit" >
                  <b-form-group horizontal
                        label="Existing Password:"
                        :label-cols="5"
                        label-for="password" class="pb-2">
                      <b-form-input type="password" placeholder="Existing Password" id="password" ref="password"
                              v-model="change_password.password" required autofocus></b-form-input>            
                    </b-form-group>
                  <b-form-group horizontal
                        label="New Password:"
                        :label-cols="5"
                        label-for="new_password">
                    <b-form-input type="password" placeholder="New Password" id="new_password"
                            v-model="change_password.new_password" required ></b-form-input>
                  </b-form-group>
                  <b-form-group horizontal
                        label="Confirm New Password:"
                        :label-cols="5"
                        label-for="new_password_confirm">
                    <b-form-input type="password" placeholder="Confirm New Password" id="new_password_confirm"
                            v-model="change_password.new_password_confirm" required ></b-form-input>
                  </b-form-group>
                </b-form>
          </b-modal>
            
        </div>
        <div class="col-10" v-if="currentHost != null">
            <div class="row">
                          <div class="col-6">
                            <b-form-group horizontal
                                label="Start Date"
                                label-for="datepicker">
                                <datepicker id="datepicker" v-model="startDate" @closed="updateHistory"></datepicker>
                            </b-form-group>
                          </div>
                          <div class="col-6">
                            <b-form-group horizontal
                                label="End Date"
                                label-for="datepicker1">
                                <datepicker id="datepicker1" v-model="endDate" @closed="updateHistory"></datepicker>
                            </b-form-group>
                        </div>
            </div>
            <div class="row">
                        <div class="col-12">
                          <host-usage-chart :chart-data="chartData" 
                          :options="chartOptions"  
                          :width="400"
                          :height="400">
                          </host-usage-chart>
                        </div>
            </div>  
              
            
            <div class="row">
              <div class="col-6">
                <b-card header="<b>Today's data</b>">
                  <b-list-group >
                      <b-list-group-item >
                        Host: {{currentHost.host}}
                      </b-list-group-item>
                      <b-list-group-item >
                        Status: {{getStatusString(currentHost)}}&nbsp;
                        <b-button v-if="currentHost.status==1" size="sm" variant="primary" @click="blockCurrentHost">Close</b-button>
                      </b-list-group-item>                      
                      <b-list-group-item >
                        Usage: {{currentHost.usage}}
                      </b-list-group-item>
                      <b-list-group-item>
                          Limit: {{currentHost.limit}}
                          <b-form>
                              <b-form-group horizontal
                                label="Add minutes"
                                label-for="minutes">
                                <b-form-input id="minutes" size="sm" v-model="add_minutes" type="number"/>
                            </b-form-group>
                            <b-button type="button" size="sm" variant="primary" @click="addMinutes">Add</b-button>
                          </b-form>
                      </b-list-group-item>
                      
                  </b-list-group>
                </b-card>                
                  
              </div>
            </div>
        </div>      
      </div>
    </div>
</template>
  
<script>
import Datepicker from 'vuejs-datepicker';
import HostUsageChart from './HostUsageChart.vue'
const moment = require('moment');

export default {
 components: {
    Datepicker,
    HostUsageChart
  },  
  data() {
      return {
        hosts : [],
        change_password: {
          password:'',
          new_password:'',
          new_password_confirm:'',
          error: null
        },
        add_minutes: 1,
        currentHost:null,
        startDate: moment().subtract(1,'months').startOf('month').toDate(),
        endDate: moment().toDate(),
        chartData: null,
        currentHostHistory : [],
        chartOptions: {responsive: true, maintainAspectRatio: false, scales:
           {yAxes: [{ticks: {beginAtZero : true}}] ,
           xAxes: [{barThickness: 10,categorySpacing: 0,type:'time',distribution: 'series',time: {unit: 'day'}}]}}
      };
  },
  computed: {
    canWeUpdatePassword: function() {
      const can =  this.change_password.password == null || this.change_password.password.length <= 0 || 
      this.change_password.new_password == null || this.change_password.new_password.length == 0 ||
      this.change_password.new_password_confirm == null || this.change_password.new_password_confirm.length == 0 ||
      this.change_password.new_password !== this.change_password.new_password_confirm;
      console.log('canWeUpdatePassword:'+can);
      return can;
    }
  },
  methods : {
    focusPassword: function(event) {
      this.$refs.password.focus();
    },    
    logout: function() {
          this.$ls.set('login_token', null);
          this.$router.push('/');
    },
    changePasswordSubmit: function() {
      this.change_password.error = null;
      this.$http.post("/api/admin/update_password",
          {
            password:this.change_password.password,
            new_password:this.change_password.new_password,
            new_password_confirm:this.change_password.new_password_confirm
          })
      .then(result => {          
          this.$ls.set('login_token', null);          
          this.$refs.changePasswordModal.hide();
          this.$router.push('/');
        },error => {
          //stuff to tell to user
          this.change_password.error = "Can't change password";
          this.$refs.changePasswordModal.show();
        });
    },
    changePassword: function() {
      this.$refs.changePasswordModal.show();
    },
    updateHostsList : function() {
      for(var i=0;i<this.hosts.length;i++) {
        if(this.hosts[i].host === this.currentHost.host) {
          this.hosts[i]=this.currentHost;
          break;
        }
      }
    },
    blockCurrentHost: function(){
      this.$http.post("/api/block",{host:this.currentHost.host}).then(result => {
          this.currentHost = result.body;
          this.updateHostsList();
        },error => {
          console.error(error);
        });
    },
    addMinutes : function () {
        if(this.currentHost == null) {
          return;
        }
        this.$http.post('/api/admin/add_host_time',{host:this.currentHost.host,minutes:this.add_minutes}).then(result => {
          this.currentHost = result.body;
          this.updateHostsList();
        },error => {
          console.error(error);
        });
    },
    getStatusString: function(host){
        if(host.status == 0) {
          return 'Closed';
        }else if (host.status == 1) {
          return 'Open';
        } else {
          return 'N/A';
        }
    },
      updateHistory: function (date) {
            if(this.currentHost == null) {
              return;
            }
            this.currentHostHistory = [];
            this.$http.get("/api/admin/host_history",
            {params:{
                      host:this.currentHost.host,
                      start_date:moment(this.startDate).format('YYYY-MM-DD'),
                      end_date:moment(this.endDate).format('YYYY-MM-DD')
                    }})
            .then(result => {
                    var labels = [];
                    result.body.forEach(element => {
                      labels.push(moment(element.day,'YYYY-MM-DD').toDate());
                      this.currentHostHistory.push({
                          x: moment(element.day,'YYYY-MM-DD').toDate(),
                          y: element.minutes
                      });                           
                    });
                    this.chartData = {labels: labels,datasets: [{label : 'Usage: '+this.currentHost.host,data: this.currentHostHistory,
                                    backgroundColor: '#FF2222AA'}]};
                    console.log(JSON.stringify(this.chartData));
                  }, error => {
                      console.error(error);
                  });    
      },
      displayHostDetails: function(host) {
          this.currentHost = host;
          this.updateHistory();                      
        },
        host_status: function(host) {
          return host.status == 1? 'success':'light';
        },
        refreshHosts : function () {
          this.$http.get("/api/admin/hosts").then(result => {
                    console.log(JSON.stringify(result.body));
                    this.hosts = result.body;
                    if(this.hosts.length == 1) {
                      this.displayHostDetails(this.hosts[0]);
                    }
                  }, error => {
                      console.error(error);
                  });          
        }
  },
  mounted() {    
    this.$nextTick(this.refreshHosts);
  }
}
</script>
<style scoped>
#admin-content {
  margin-top: 5rem;
  padding-left: 1rem;
  padding-right: 1rem;
}

</style>