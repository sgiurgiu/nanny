<script>
import { Bar,mixins } from 'vue-chartjs';
const { reactiveProp } = mixins

export default {
  extends: Bar,
   mixins: [reactiveProp],
  name: 'HostUsageChart',
  props: ['chartData','options'],
  mounted () {
      console.log('data:'+JSON.stringify(this.chartData));
    this.renderChart(this.chartData, this.options);
  }
}

</script>

<style scoped>
</style>