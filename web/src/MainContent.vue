<template>
 <div id = "main-content">
    <b-container>
        <b-jumbotron :header="status_message"
                     :lead="time_message">
          <b-button :disabled="button_disabled" variant="primary" class="my-2 my-sm-0" @click="changeHostStatus">{{button_text}}</b-button>
        </b-jumbotron>
        
      </b-container>
 </div>
</template>
<script>
export default {
  name: "MainContent",
  data() {
    return {
      status_message: "Internet status",
      time_message : "Time N/A",
      host_status: -1,
      button_text:"N/A",
      button_disabled:false,
      timeout:null
    }
  },
  methods : {
    changeHostStatus : function() {
      var changeSuccess = function () {
        this.button_disabled = false;
        this.displayStatus();
      };
      this.button_disabled = true;
      if(this.host_status == 0 ) {
        this.$http.post("/api/allow").then(changeSuccess);
      } else if(this.host_status == 1) {
        this.$http.post("/api/block").then(changeSuccess);
      }
    },
    displayStatus : function () {
       var minutesToStr = function(min) {
        if(min < 0) return "N/A";
        var hours = ~~(min/60);
        var minutes = Math.floor(min%60);
        return hours+"H:"+minutes+"M";
      };
      this.$http.get("/api/block_status").then(result => {
          this.status_message = "Internet is ";
          this.host_status = result.data.status;
          switch(result.data.status) {
            case 0:
              this.status_message += 'closed';
              this.button_text = "Open";
            break;
            case 1:
              this.status_message += 'open';
              this.button_text = "Close";
            break;
            default:
              this.status_message += 'N/A';
              this.button_text = "N/A";
            break;
          };
          this.time_message="Today you have used "+minutesToStr(result.data.usage)+". This limit is "+minutesToStr(result.data.limit);         
          
        }, error => {
            console.error(error);
        });
    }
  },
  mounted() {
    var self = this;
    this.displayStatus();
    this.timeout =  setInterval(() => {
           this.displayStatus();
    },5000);
    
    
  },
  beforeDestroy() {
     clearInterval(this.timeout);    
  }  
}
</script>

<style scoped>
#main-content {
  margin-top: 5rem;
}

</style>
