<!--Load the AJAX API-->
<script type="text/javascript" src="https://www.gstatic.com/charts/loader.js"></script>
<!--jquery-->
<script src="https://code.jquery.com/jquery-3.3.1.min.js"></script>
<script type="text/javascript">
  // Load the Visualization API and the corechart package.
  google.charts.load('current', {'packages':['corechart', 'controls']});

  // Set a callback to run when the Google Visualization API is loaded.
  //google.charts.setOnLoadCallback(load_data);

  function drawDashboard(dbResponse, type){
    var data = new google.visualization.DataTable();
    data.addColumn('date', 'Date'); // X-axis
    data.addColumn('number', 'Temp');
    data.addColumn('number', 'Humidity');
    data.addColumn('number', 'Pressure');
    data.addColumn('number', 'Illuminance');
    data.addColumn('number', 'CO');

    var jsonData = $.parseJSON(dbResponse);

    for (var i = 0; i < jsonData.length; i++) {
      data.addRow([new Date(jsonData[i].timestamp.replace(" ", "T")), parseInt(jsonData[i].temperature), parseInt(jsonData[i].humidity), parseInt(jsonData[i].pressure), parseInt(jsonData[i].illuminance), parseInt(jsonData[i].cmonoxide)]);
    }

    // ===============
    // CHART OPTIONS
    // ===============
    //Options for the myDateSlider. Also acts as default for charts if not overwritten
    var defaults = {
      colors:['#b52c15','#00485e', '#442151','#383700', '#699199'],
      vAxes: {
        0:{gridlines: {count: 5}},
        1:{gridlines: {count: 5, color: 'transparent'}},
        2:{gridlines: {count: 5, color: 'transparent'}},
        3:{gridlines: {count: 5, color: 'transparent'}},
        4:{gridlines: {count: 5, color: 'transparent'}},
      },
      series: {
        0: {type: 'line', targetAxisIndex: 0},
        1: {type: 'line', targetAxisIndex: 1},
        2: {type: 'line', targetAxisIndex: 2},
        3: {type: 'line', targetAxisIndex: 3},
        4: {type: 'line', targetAxisIndex: 4}
      },
      chartArea : {width:'90%', height:'85%'},
      backgroundColor: 'transparent',
      hAxis: {
        gridlines: {
          color: 'grey'
        },
        minorGridlines: {
          count: 0
        },
        textStyle: {
          color: 'grey'
        }
      },
      vAxis: {
        titleTextStyle:{
          color: 'grey'
        },
        gridlines: {
          color: 'grey'
        },
        minorGridlines: {
          count: 0
        },
        textStyle: {
          color: 'grey'
        }
      },
      legend: {
        position: 'top',
        textStyle: {
          color: 'grey'
        }
      }
    }

    var chart1 = {
      colors:['#b52c15','#283800'],
      vAxes: {
        0:{title: "Temperature", gridlines: {count: 5}},
        1:{title: "Illuminance",  gridlines: {count: 5, color: 'transparent'}},
      },
      series: {
        0: {type: 'line', targetAxisIndex: 0, enableInteractivity: true},
        1: {type: 'bars', targetAxisIndex: 1, enableInteractivity: true, dataOpacity: 0.8}
      },
      bar: {groupWidth:"100%"}
    }
    var chart2={
      colors:['#00485e', '#442151'],
      vAxes: {
        0:{title: "Humidity", gridlines: {count: 5}},
        1:{title: "Pressure",  gridlines: {count: 5, color: 'transparent'}},
      },
      series: {
        0: {type: 'line', targetAxisIndex: 0, enableInteractivity: true},
        1: {type: 'bars', targetAxisIndex: 1, enableInteractivity: true, dataOpacity: 0.8}
      },
      bar: {groupWidth:"100%"}
    }
    var chart3={
      colors:['#699199'],
      vAxes: {
        0:{title: "Carbon monoxide", gridlines: {count: 5}},
      },
      series: {
        0: {type: 'line', targetAxisIndex: 0, enableInteractivity: true},
      }
    }

    //Combine options
    var chart1opts = $.extend({}, defaults, chart1);
    var chart2opts = $.extend({}, defaults, chart2);
    var chart3opts = $.extend({}, defaults, chart3)

    var myDashboard = new google.visualization.Dashboard(document.getElementById('dashboard_div'));

    var myDateSlider = new google.visualization.ControlWrapper({
      'controlType': 'ChartRangeFilter',
      'containerId': 'control_div',
      'options': {
        'filterColumnLabel': 'Date',
        ui: {
          chartOptions: defaults
        }
      }
    });
    var myChart = new google.visualization.ChartWrapper({
      view: {'columns': [0,1,4]},
      chartType : 'ComboChart',
      containerId : 'chart_div',
      options : chart1opts
    });
    var myChart2 = new google.visualization.ChartWrapper({
      view: {'columns': [0,2,3]},
      chartType : 'LineChart',
      containerId : 'chart2_div',
      options : chart2opts
    });
    var myChart3 = new google.visualization.ChartWrapper({
      view: {'columns': [0,5]},
      chartType : 'LineChart',
      containerId : 'chart3_div',
      options : chart3opts
    });
    // Bind myLine to the dashboard, and to the controls
    // this will make sure our line chart is update when our date changes
    myDashboard.bind(myDateSlider, [myChart, myChart2, myChart3]);
    myDashboard.draw(data);

    var hideTemp = document.getElementById("chkbox_tmp");
    hideTemp.onclick = function(){
      hideLine(this.checked, 0, chart1);
      hideLine(this.checked, 0, defaults);
    }
    var hideIllu = document.getElementById("chkbox_illu");
    hideIllu.onclick = function(){
      hideBars(this.checked, 1, chart1);
      hideLine(this.checked, 3, defaults);
    }
    var hidePres = document.getElementById("chkbox_pres");
    hidePres.onclick = function(){
      hideBars(this.checked, 1, chart2);
      hideLine(this.checked, 2, defaults);
    }
    var hideHumi = document.getElementById("chkbox_humi");
    hideHumi.onclick = function(){
      hideLine(this.checked, 0, chart2);
      hideLine(this.checked, 1, defaults);
    }
    var hideCO = document.getElementById("chkbox_co");
    hideCO.onclick = function(){
      hideLine(this.checked, 0, chart3);
      hideLine(this.checked, 4, defaults);
    }

    function hideLine(checked, series, chart){
      view = new google.visualization.DataView(data);
      if (checked){
        chart.series[series].lineWidth = 2.0;
        chart.series[series].enableInteractivity = true;
      } else {
        chart.series[series].lineWidth = 0.0;
        chart.series[series].enableInteractivity = false;
      }
      myDashboard.draw(view);
    }

    function hideBars(checked, series, chart){
      view = new google.visualization.DataView(data);
      if (checked){
        chart.series[series].dataOpacity = 0.8;
        chart.series[series].enableInteractivity = true;
      } else {
        chart.series[series].dataOpacity = 0.0;
        chart.series[series].enableInteractivity = false;
      }
      myDashboard.draw(view);
    }

  }
</script>
<script>
  $(function () {
    $('.update_charts').click(function(){
      //Get the data from database
      $.ajax({
        type: 'POST',
        url: '<?php echo site_url('stations/ajaxtest'); ?>',
        data: $(this).parent().serialize(),
        success: function (dbResponse) {
          drawDashboard(dbResponse);  //Draw the chart and give the data to it.
        }
      });
      return false; // avoid to execute the actual form submission. AJAX will handle this
    });
  });

</script>

<div id="main" style="width:85%">
  <p><b>Search station by ID and optionally specify a timeframe.
     By default data is fetched from past 7 days.
  </b></p>
  <form id="visform" method="post">
    <table>
      <tr>
        <td>Id</td><td><input id="station_id" type="text" name="id" size="5" required></td>
        <td>Start</td><td><input type="date" name="start"></td><td>End</td><td><input type="date" name="end"></td>
      </tr>
      </table>
      <input type="submit" class="update_charts" value="Draw">
  </form>
  <br>

  <input id="chkbox_tmp" type="checkbox" name="temperature" checked>Temperature
  <input id="chkbox_illu" type="checkbox" name="illuminance" checked> Illuminance
  <input id="chkbox_pres" type="checkbox" name="pressure" checked> Pressure
  <input id="chkbox_humi" type="checkbox" name="humidity" checked> Humidity
  <input id="chkbox_co" type="checkbox" name="cmonoxide" checked> CO

  <br>
  <div id="dashboard_div" style=""></div>
    <div id="control_div" style="height: 130px"></div>
    <div id="chart_div"  style="display:inline-block; width:95%; height: 350px"></div>
    <div id="chart2_div" style="display:inline-block; width:95%;height: 350px"></div>
    <div id="chart3_div" style="display:inline-block; width:95%;height: 350px"></div>
</div>
