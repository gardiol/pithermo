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
    "dojox/charting/action2d/Tooltip",
    "dojox/charting/action2d/Magnify",
    "dojox/charting/widget/Legend",
    "dojo/domReady!"], 
function( dom, attr, dclass, style, html, on,// Dojo
          CheckBox, Select, DateTextBox, DijitTooltip,// Dijit
          Chart, Default, Lines, StackedColumns,StackedAreas,Chris, Areas, Markers, Tooltip, Magnify, Legend )// Charing
{
    sta = { 
            hStart: null,
            hEnd: null,
			timer: null,
            grpRef: {},
    		data:  {},
            totalP: 0,
            totalPl:0,
            totalG:0,
        
            grp: new Chart("stats-graph",{ title: "Statistiche funzionamento", titlePos: "bottom", titleGap: 25}),

            toggleRange: function(){
                dclass.toggle(dom.byId("stats-range"), "hidden");
                sta.update();
            },

            drawGraph: function(){
				var axna = { 0:"Pellet", 1:"PelletLow", 2:"Gas", 3:"Mins", 4:"Maxs", 5:"Avgs", 6:"MinsE", 7:"MaxsE", 8:"AvgsE" };
                var show = { 0:dom.byId("stats-show-a").checked&&dom.byId("stats-show-p").checked,
                             1:dom.byId("stats-show-a").checked&&dom.byId("stats-show-pl").checked,
                             2:dom.byId("stats-show-a").checked&&dom.byId("stats-show-g").checked,
                             3:dom.byId("stats-show-ti").checked&&dom.byId("stats-show-tmin").checked, 
                             4:dom.byId("stats-show-ti").checked&&dom.byId("stats-show-tmax").checked, 
                             5:dom.byId("stats-show-ti").checked&&dom.byId("stats-show-tavg").checked,
                             6:dom.byId("stats-show-te").checked&&dom.byId("stats-show-tmin").checked, 
                             7:dom.byId("stats-show-te").checked&&dom.byId("stats-show-tmax").checked, 
                             8:dom.byId("stats-show-te").checked&&dom.byId("stats-show-tavg").checked };
				var list = { 0:[], 1:[], 2:[], 3:[], 4:[], 5:[], 6:[], 7:[], 8:[] };
                var tots = [];
                var x = 1;
                for (var time in sta.data){
                    sta.grpRef[x] = time;
                    var tot = 0;
					for ( var n in axna ){                    
						var val = sta.data[time][n];
                        if ( show[n] ){
                            list[n].push( val );
                            if ( n < 3 )
                                tot += val;
                        }
					}
					tots.push( tot );
					x++;
                }
				for ( var n in axna )
                    sta.grp.updateSeries( axna[n], list[n] );
                sta.grp.updateSeries( "Totals", tots );
				html.set(dom.byId("stats-total-pellet"), "Pellet acceso per: " + (sta.totalP+sta.totalPl).toFixed(1) + "h" );
				html.set(dom.byId("stats-total-pellet-mod"), " (in modulazione per: " + sta.totalP.toFixed(1) + "h" );
				html.set(dom.byId("stats-total-pellet-min"), " e al minimo per: " + sta.totalPl.toFixed(1) + "h)" );
				html.set(dom.byId("stats-total-gas"), "Gas acceso per: " + sta.totalG.toFixed(1) + "h" );
        		sta.grp.render(); 
			},
			
            clearData: function(){
                sta.data = {};
                sta.grpRef = {};
                sta.totalP = sta.totalPl = sta.totalG = 0;
                sta.drawGraph();                                
            },
        
            setData: function(new_rows){
                var len = new_rows.length;
                var pos = 0;
                while ( pos < len ){
                    var line_end = new_rows.indexOf( "\n", pos );
					if ( line_end != -1 ){
						var row = new_rows.substr( pos, line_end-pos+1 );
						var split_row = row.split(" ");
                        sta.data[ parseInt(split_row[0]) ] = {0: parseFloat(split_row[1] / 3600.0), // pellet time
                                                             1: parseFloat(split_row[2] / 3600.0), // pellet min time
                                                             2: parseFloat(split_row[3] / 3600.0), // gas time
                                                             3: parseFloat(split_row[4]), // t min
                                                             4: parseFloat(split_row[5]), // t max
                                                             5: parseFloat(split_row[6]), // t avg
                                                             6: parseFloat(split_row[7]), // t ext min
                                                             7: parseFloat(split_row[8]), // t ext max
                                                             8: parseFloat(split_row[9])} // t ext avg
						pos = line_end+1;
					} else {
						var row = new_rows.substr( pos );
						var split_row = row.split(" ");
                        sta.totalP = parseFloat(split_row[1] / 3600.0);
                        sta.totalPl= parseFloat(split_row[2] / 3600.0);
                        sta.totalG = parseFloat(split_row[3] / 3600.0);
						pos = len;
					 }
                }
                sta.drawGraph();
            },
        
 			update: function(){
				if ( sta.timer ){
                    window.clearTimeout( sta.timer );
                    sta.timer = null;
				}
                if ( !statsUseRange.checked ){
                    sta.hStart = new Date(new Date() - 1000*60*60*24*15); 
                    sta.hEnd = new Date();
                    statsEnd.set("value", sta.hEnd );
                    statsStart.set("value", sta.hStart );
                } else {
                    sta.hEnd = statsEnd.get("value")
                    sta.hStart = statsStart.get("value")
                }
                var sDate = sta.hStart;sDate.setHours(0);sDate.setMinutes(0);sDate.setSeconds(0);
                var eDate = sta.hEnd;eDate.setHours(23);eDate.setMinutes(59);eDate.setSeconds(59);
				var startDate = Math.floor( sDate / 1000 );
				var endDate = Math.floor( eDate / 1000 ); 

                sta.clearData();
                postRequest("cgi-bin/stats",startDate+":"+endDate,
                    function(result){
                        if ( result )
                            sta.setData(result);
						if ( !statsUseRange.checked )
							sta.timer = window.setTimeout( function(){ sta.update(); }, 60 * 1000 * 15);
                    },
                    function(err){
						sta.timer = window.setTimeout( function(){ sta.update(); }, 60 * 1000 * 15);
					});                    
			}
		};
    
    
	sta.grp.setTheme(Chris);
    
	sta.grp.addPlot("TimePlot", {type: StackedColumns, gap: 5 });     
    sta.grp.addAxis("x", { plot: "TimePlot", labelFunc: function(t,v,p){return utils.date2str(sta.grpRef[v]*1000)} });
    sta.grp.addAxis("y", { plot: "TimePlot", vertical:true, min: 0 });
    sta.grp.addSeries("Pellet", [1,2,3,4,5,6,7,8,9,10],{  plot: "TimePlot", /*stroke: "black", fill: "purple", */minorThicks: false, legend: "Ore pellet modulazione" } );
	sta.grp.addSeries("PelletLow", [1,2,3,4,5,6,7,8,9,10],{ plot: "TimePlot",/*stroke: "black", fill: "blue", */legend: "Ore pellet minimo" } );
    sta.grp.addSeries("Gas", [1,2,3,4,5,6,7,8,9,10],{plot: "TimePlot",/*stroke: "black", fill: "yellow",*/ legend: "Ore gas acceso" } );
    sta.grp.connectToPlot("TimePlot",
        function(evt){
            if ( evt.type == "onclick" ){
                    var lt = sta.grp.getCoords();
                    var aroundRect = {type: "rect"};
                    aroundRect.x = Math.round(evt.cx + lt.x);
                    aroundRect.y = Math.round(evt.cy + lt.y);
                    aroundRect.w = aroundRect.h = 1;                    
                    DijitTooltip.show("<div style='text-align:center;'>"+evt.y.toFixed(1)+"h</div>", aroundRect);
            }
        });

    sta.grp.addPlot("TotPlot", {type: Lines, lines: false, markers: true});     
    sta.grp.addAxis("y", { plot: "TotPlot", vertical:true, min: 0 });
    sta.grp.addSeries("Totals", [1,2,3,4,5,6,7,8,9,10],{  plot: "TotPlot", legend:"" } );
    new Magnify( sta.grp, "TotPlot");

    sta.grp.movePlotToFront( "TotPlot" );
    sta.grp.connectToPlot("TotPlot",
        function(evt){
            if ( evt.type == "onclick" ){
                    var lt = sta.grp.getCoords();
                    var aroundRect = {type: "rect"};
                    aroundRect.x = Math.round(evt.cx + lt.x);
                    aroundRect.y = Math.round(evt.cy + lt.y);
                    aroundRect.w = aroundRect.h = 1;                    
                    DijitTooltip.show("<div style='text-align:center;'>"+evt.y.toFixed(1)+"h</div>", aroundRect);
            }
        });

    sta.grp.addPlot("TempPlot", {hAxis: "x", vAxis:"t", type: Lines, lines: true, markers: true});     
    sta.grp.addAxis("t", { plot: "TempPlot", vertical:true, leftBottom: false,
                            majorTickStep: 5, majorTicks: true, majorLabels: true,
                            minorTickStep: 1, minorTicks: true, minorLabels: false,
                            microTickStep: 0.1, microTicks: false });
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
                    var lt = sta.grp.getCoords();
                    var aroundRect = {type: "rect"};
                    aroundRect.x = Math.round(evt.cx + lt.x);
                    aroundRect.y = Math.round(evt.cy + lt.y);
                    aroundRect.w = aroundRect.h = 1;                    
                    DijitTooltip.show("<div style='text-align:center;'>"+evt.y.toFixed(1)+"°</div>", aroundRect);
            }
        });
    new Magnify( sta.grp, "TempPlot");
        
    sta.grp.render(); 
    new Legend({chartRef:sta.grp, horizontal:3}, 'stats-legend');

    
});
