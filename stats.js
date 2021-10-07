var hst = null;

require([
    "dojo/dom", 
    "dojo/dom-attr",
    "dojo/dom-class",
    "dojo/dom-style",
    "dojo/html",
    "dojo/on",
    "dijit/form/CheckBox",
    "dijit/form/Select",
    "dijit/form/DateTextBox",
    "dijit/Tooltip",
    "dojox/charting/Chart",
    "dojox/charting/axis2d/Default", 
    "dojox/charting/plot2d/Lines",
    "dojox/charting/plot2d/StackedColumns",
    "dojox/charting/plot2d/StackedAreas",
    "dojox/charting/themes/Chris",
    "dojox/charting/plot2d/Areas",
    "dojox/charting/plot2d/Markers",
    "dojox/charting/plot2d/Grid",
    "dojox/charting/action2d/Tooltip",
    "dojox/charting/action2d/Magnify",
    "dojox/charting/widget/Legend",
    "dojo/domReady!"], 
function( dom, attr, dclass, style, html, on,// Dojo
          CheckBox, Select, DateTextBox, DijitTooltip,// Dijit
          Chart, Default, Lines, StackedColumns,StackedAreas,Chris, Areas, Markers, Grid, Tooltip, Magnify, Legend )// Charing
{
    sta = { 
        axNames: { 0:"Pellet", 1:"PelletLow", 2:"Gas", 3:"Mins", 4:"Maxs", 5:"Avgs", 6:"MinsE", 7:"MaxsE", 8:"AvgsE", 9:"Totals" },
        data: { 0:[1,1,1], 1:[1,1,1], 2:[1,1,1], 3:[1,1,1], 4:[0,0,0], 5:[0,0,0], 6:[0,0,0], 7:[0,0,0], 8:[0,0,0], 9:[0,0,0] },
        timeRef: [0,1,2],
        ttipRect: null,
        hStart: null,
        hEnd: null,
        
        grp: new Chart("stats-graph",{ title: "Statistiche funzionamento", titlePos: "bottom", titleGap: 25}),

        toggleTempi: function(){
            if ( stats_show_tempi.checked ){
                sta.grp.updateSeries( sta.axNames[0], stats_show_tempi_pellet_mod.checked ? sta.data[0] : [] );
                sta.grp.updateSeries( sta.axNames[1], stats_show_tempi_pellet_min.checked ? sta.data[1] : [] );
                sta.grp.updateSeries( sta.axNames[2], stats_show_tempi_gas.checked ? sta.data[2] : [] );
                stats_show_tempi_pellet_mod.set("disabled", false );
                stats_show_tempi_pellet_min.set("disabled", false );
                stats_show_tempi_gas.set("disabled", false );
            } else {
                sta.grp.updateSeries( sta.axNames[0], [] );    
                sta.grp.updateSeries( sta.axNames[1], [] );    
                sta.grp.updateSeries( sta.axNames[2], [] );    
                stats_show_tempi_pellet_mod.set("disabled", true );
                stats_show_tempi_pellet_min.set("disabled", true );
                stats_show_tempi_gas.set("disabled", true );
            }
            sta.grp.render();        
        },

        toggleTempExt: function(){
            if ( stats_show_temp_ext.checked ){
                sta.grp.updateSeries( sta.axNames[6], stats_show_temp_min.checked ? sta.data[6] : [] );    
                sta.grp.updateSeries( sta.axNames[7], stats_show_temp_max.checked ? sta.data[7] : [] );
                sta.grp.updateSeries( sta.axNames[8], stats_show_temp_avg.checked ? sta.data[8] : [] );
                stats_show_temp_min.set("disabled", false );
                stats_show_temp_avg.set("disabled", false );
                stats_show_temp_max.set("disabled", false );
            } else {
                sta.grp.updateSeries( sta.axNames[6], [] );    
                sta.grp.updateSeries( sta.axNames[7], [] );    
                sta.grp.updateSeries( sta.axNames[8], [] );
                if ( !stats_show_temp_int.checked ){
                    stats_show_temp_min.set("disabled", true );
                    stats_show_temp_avg.set("disabled", true );
                    stats_show_temp_max.set("disabled", true );
                }
            }
            sta.grp.render();        
        },

        toggleTempInt: function(){
            if ( stats_show_temp_int.checked ){
                sta.grp.updateSeries( sta.axNames[3], stats_show_temp_min.checked ? sta.data[3] : [] );    
                sta.grp.updateSeries( sta.axNames[4], stats_show_temp_max.checked ? sta.data[4] : [] );
                sta.grp.updateSeries( sta.axNames[5], stats_show_temp_avg.checked ? sta.data[5] : [] );
                stats_show_temp_min.set("disabled", false );
                stats_show_temp_avg.set("disabled", false );
                stats_show_temp_max.set("disabled", false );
            } else {
                sta.grp.updateSeries( sta.axNames[3], [] );    
                sta.grp.updateSeries( sta.axNames[4], [] );    
                sta.grp.updateSeries( sta.axNames[5], [] );   
                if ( !stats_show_temp_ext.checked ){ 
                    stats_show_temp_min.set("disabled", true );
                    stats_show_temp_avg.set("disabled", true );
                    stats_show_temp_max.set("disabled", true );
                }
            }
            sta.grp.render();        
        },

        toggleTempMin: function(){
            if ( stats_show_temp_min.checked ){
                sta.grp.updateSeries( sta.axNames[3], stats_show_temp_int.checked ? sta.data[3] : [] );    
                sta.grp.updateSeries( sta.axNames[6], stats_show_temp_ext.checked ? sta.data[6] : [] );
            } else {
                sta.grp.updateSeries( sta.axNames[3], [] );    
                sta.grp.updateSeries( sta.axNames[6], [] );    
            }
            sta.grp.render();        
        },

        toggleTempMax: function(){
            if ( stats_show_temp_max.checked ){
                sta.grp.updateSeries( sta.axNames[4], stats_show_temp_int.checked ? sta.data[4] : [] );    
                sta.grp.updateSeries( sta.axNames[7], stats_show_temp_ext.checked ? sta.data[7] : [] );
            } else {
                sta.grp.updateSeries( sta.axNames[4], [] );    
                sta.grp.updateSeries( sta.axNames[7], [] );    
            }
            sta.grp.render();        
        },

        toggleTempAvg: function(){
            if ( stats_show_temp_avg.checked ){
                sta.grp.updateSeries( sta.axNames[5], stats_show_temp_int.checked ? sta.data[5] : [] );    
                sta.grp.updateSeries( sta.axNames[8], stats_show_temp_ext.checked ? sta.data[8] : [] );
            } else {
                sta.grp.updateSeries( sta.axNames[5], [] );    
                sta.grp.updateSeries( sta.axNames[8], [] );    
            }
            sta.grp.render();        
        },

        toggleRange: function(){
            dclass.toggle(dom.byId("stats-range"), "hidden");
            if ( !statsUseRange.checked )
                sta.update();
        },

        drawSerie: function(s,v){            
            sta.grp.updateSeries( sta.axNames[s], v ? sta.data[s] : [] );    
            sta.grp.render();        
        },
                
        update: function(){
            if ( !statsUseRange.checked ){
                var ds = new Date()  - 1000*60*60*24*8;
                statsStart.set("value", new Date( ds ) );
                statsEnd.set("value", new Date() );
            }
            var sDate = statsStart.get("value");sDate.setHours(0);sDate.setMinutes(0);sDate.setSeconds(5);
            var eDate = statsEnd.get("value");eDate.setHours(23);eDate.setMinutes(59);eDate.setSeconds(55);
            var startDate = Math.floor( sDate / 1000 );
            var endDate = Math.floor( eDate / 1000 ); 

            postRequest("cgi-bin/stats",startDate+":"+endDate,
                function(new_rows){
                    if ( new_rows ){
                        var len = new_rows.length;
                        var pos = 0;
                        sta.data = { 0:[], 1:[], 2:[], 3:[], 4:[], 5:[], 6:[], 7:[], 8:[], 9:[] };
                        sta.timeRef = [];
                        while ( pos < len ){
                            var line_end = new_rows.indexOf( "\n", pos );
                            if ( line_end != -1 ){
                                var row = new_rows.substr( pos, line_end-pos+1 );
                                var split_row = row.split(" ");
                                sta.timeRef.push( parseFloat(split_row[0])*1000 ); // time
                                var pellet_tot = parseFloat(split_row[1]);
                                var pellet_low = parseFloat(split_row[2]);
                                var gas_time = parseFloat(split_row[3]); // gas time
                                sta.data[0].push( (pellet_tot - pellet_low) / 3600.0 );// pellet mod
                                sta.data[1].push( pellet_low / 3600.0 );// pellet min
                                sta.data[2].push( gas_time / 3600.0 );// gas
                                sta.data[3].push( parseFloat(split_row[4]) ); // t min
                                sta.data[4].push( parseFloat(split_row[5]) ); // t max
                                sta.data[5].push( parseFloat(split_row[6]) ); // t avg
                                sta.data[6].push( parseFloat(split_row[7]) ); // t ext min
                                sta.data[7].push( parseFloat(split_row[8]) ); // t ext max
                                sta.data[8].push( parseFloat(split_row[9]) ); // t ext avg
                                sta.data[9].push( (pellet_tot + gas_time) / 3600.0 ); // totals

                                pos = line_end+1;
                            } else {
                                var row = new_rows.substr( pos );
                                var split_row = row.split(" ");
                                var totalP = parseFloat(split_row[1] / 3600.0);
                                var totalPl= parseFloat(split_row[2] / 3600.0);
                                var totalG = parseFloat(split_row[3] / 3600.0);
                                html.set(dom.byId("stats-total-pellet"), (totalP+totalPl).toFixed(1) + "h" );
                                html.set(dom.byId("stats-total-pellet-mod"), totalP.toFixed(1) + "h" );
                                html.set(dom.byId("stats-total-pellet-min"), totalPl.toFixed(1) + "h)" );
                                html.set(dom.byId("stats-total-gas"), totalG.toFixed(1) + "h" );
                                pos = len;
                                }
                        }
                        sta.grp.updateSeries( sta.axNames[0], stats_show_tempi.checked&&stats_show_tempi_pellet_mod.checked ? sta.data[0] : [] );    
                        sta.grp.updateSeries( sta.axNames[1], stats_show_tempi.checked&&stats_show_tempi_pellet_min.checked ? sta.data[1] : [] );    
                        sta.grp.updateSeries( sta.axNames[2], stats_show_tempi.checked&&stats_show_tempi_gas.checked ? sta.data[2] : [] );    
                        sta.grp.updateSeries( sta.axNames[3], stats_show_temp_int.checked&&stats_show_temp_min.checked ? sta.data[3] : [] );    
                        sta.grp.updateSeries( sta.axNames[4], stats_show_temp_int.checked&&stats_show_temp_max.checked ? sta.data[4] : [] );    
                        sta.grp.updateSeries( sta.axNames[5], stats_show_temp_int.checked&&stats_show_temp_avg.checked ? sta.data[5] : [] );    
                        sta.grp.updateSeries( sta.axNames[6], stats_show_temp_ext.checked&&stats_show_temp_min.checked ? sta.data[6] : [] );    
                        sta.grp.updateSeries( sta.axNames[7], stats_show_temp_ext.checked&&stats_show_temp_max.checked ? sta.data[7] : [] );    
                        sta.grp.updateSeries( sta.axNames[8], stats_show_temp_ext.checked&&stats_show_temp_avg.checked ? sta.data[8] : [] );    
                        sta.grp.updateSeries( sta.axNames[9], stats_show_tempi.checked ? sta.data[9] : [] );
                        sta.grp.render(); 
                    }
                },
                function(err){
                });                    
        },
        showTip:function(r,t,p){
                var lt = sta.grp.getCoords();
                sta.ttipRect = {type: "rect"};
                sta.ttipRect.x = Math.round(r.x + lt.x);
                sta.ttipRect.y = Math.round(r.y + lt.y);
                sta.ttipRect.w = sta.ttipRect.h = 1;    
                DijitTooltip.show(t, sta.ttipRect,p);
        },
        hideTip:function(){
                if ( sta.ttipRect != null ){
                    DijitTooltip.hide(sta.ttipRect);
                    sta.ttipRect = null;
                }
        },
        
        startup: function(){
            sta.grp.setTheme(Chris);
            
            sta.grp.addPlot("TimePlot", {type: StackedColumns, gap: 5,  hAxis:"x", vAxis:"y" });
            sta.grp.addAxis("x", { plot: "TimePlot", labelFunc: function(t,v,p){return utils.date2str(sta.timeRef[v-1])} });
            sta.grp.addAxis("y", { plot: "TimePlot", vertical:true, min: 0,
                        majorTickStep: 5, majorTicks: true, majorLabels: true,
                        minorTickStep: 1, minorTicks: true, minorLabels: true,
            });
            sta.grp.addSeries("Pellet", [1,2,3,4,5,6,7,8,9,10],{  plot: "TimePlot", minorThicks: false, legend: "Ore pellet modulazione" } );
            sta.grp.addSeries("PelletLow", [1,2,3,4,5,6,7,8,9,10],{ plot: "TimePlot",legend: "Ore pellet minimo" } );
            sta.grp.addSeries("Gas", [1,2,3,4,5,6,7,8,9,10],{plot: "TimePlot", legend: "Ore gas acceso" } );
            sta.grp.connectToPlot("TimePlot",
                function(evt){
                    if ( evt.type == "onclick" ){
                        sta.showTip( evt.shape.shape, evt.y.toFixed(1)+"h", ["above"] );
                    } else if(evt.type === "onplotreset" || evt.type === "onmouseout"){
                        sta.hideTip();
                    }
                });

            sta.grp.addPlot("TotPlot", {type: Markers, lines: false, markers: true,  hAxis:"x", vAxis:"y"});     
            sta.grp.addSeries("Totals", [1,2,3,4,5,6,7,8,9,10],{  plot: "TotPlot", legend:"" } );
            new Magnify( sta.grp, "TotPlot");

            sta.grp.movePlotToFront( "TotPlot" );
            sta.grp.connectToPlot("TotPlot",
                function(evt){
                    if ( evt.type == "onclick" ){
                        sta.showTip( {x:evt.cx,y:evt.cy}, evt.y.toFixed(1)+"h",["after-centered", "before-centered"] );
                    } else if(evt.type === "onplotreset" || evt.type === "onmouseout"){
                        sta.hideTip();
                    }
                });

            sta.grp.addPlot("TempPlot", {hAxis: "x", vAxis:"t", type: Lines, lines: true, markers: true});     
            sta.grp.addAxis("t", { plot: "TempPlot", vertical:true, leftBottom: false,
                                    majorTickStep: 5, majorTicks: true, majorLabels: true,
                                    minorTickStep: 1, minorTicks: true, minorLabels: true });
            sta.grp.addSeries("Mins", [1,2,3,4,5,6,7,8,9,10],{  plot: "TempPlot", legend:"Minima interna" } );
            sta.grp.addSeries("Maxs", [1,2,3,4,5,6,7,8,9,10],{  plot: "TempPlot", legend:"Massima interna" } );
            sta.grp.addSeries("Avgs", [1,2,3,4,5,6,7,8,9,10],{  plot: "TempPlot", legend:"Media interna" } );
            sta.grp.addSeries("MinsE", [1,2,3,4,5,6,7,8,9,10],{  plot: "TempPlot", legend:"Minima esterna" } );
            sta.grp.addSeries("MaxsE", [1,2,3,4,5,6,7,8,9,10],{  plot: "TempPlot", legend:"Massima esterna" } );
            sta.grp.addSeries("AvgsE", [1,2,3,4,5,6,7,8,9,10],{  plot: "TempPlot", legend:"Media esterna" } );
            sta.grp.movePlotToFront( "TempPlot" );
            sta.grp.connectToPlot("TempPlot",
                function(evt){
                    if ( evt.type == "onclick" ){
                        sta.showTip( {x:evt.cx,y:evt.cy}, evt.y.toFixed(1)+"°",["after-centered", "before-centered"] );
                    } else if(evt.type === "onplotreset" || evt.type === "onmouseout"){
                        sta.hideTip();
                    }
                });
            new Magnify( sta.grp, "TempPlot");
                                    
            sta.grp.addPlot("gridTimePlot", { type: Grid, hAxis: "x",vAxis: "y",
                    hMajorLines: true,
                    hMinorLines: true,
                    vMajorLines: false,
                    vMinorLines: false,
                    majorHLine: { color: "gray", width: 1, style: "dash"},
                    minorHLine: { color: "gray", width: 1, style: "dot" } });

            sta.grp.addPlot("gridTempPlot", { type: Grid, hAxis: "x",vAxis: "t",
                    hMajorLines: true,
                    hMinorLines: true,
                    vMajorLines: false,
                    vMinorLines: false,
                    majorHLine: { color: "blue", width: 1, style: "dash"},
                    minorHLine: { color: "blue", width: 1, style: "dot" } });

            sta.grp.render(); 
            new Legend({chartRef:sta.grp, horizontal:3}, 'stats-legend');

//            sta.update();
        }
    };
});
