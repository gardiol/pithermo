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
    "dojox/charting/Chart",
    "dojox/charting/axis2d/Default", 
    "dojox/charting/plot2d/Lines",
    "dojox/charting/themes/Chris",
    "dojox/charting/plot2d/Areas",
    "dojox/charting/plot2d/Markers",
    "dojox/charting/action2d/MouseIndicator",
    "dojo/domReady!"], 
function( dom, attr, dclass, style, html, on,// Dojo
          CheckBox, Select, DateTextBox,// Dijit
          Chart, Default, Lines, Chris, Areas, Markers, MouseIndicator )// Charing
{
    hst = { 
            hStart: null,
            hEnd: null,
			timer: null,
    		data:  {},
        
            grp: new Chart("history-graph",{ title: "Storico", titlePos: "bottom", titleGap: 25}),

            toggleRange: function(){
                dclass.toggle(dom.byId("history-range"), "celated");
                hst.update();
            },
        
            drawGraph: function(){
				var mins = { 0:NaN,1:NaN,2:NaN,3:NaN };
				var maxs = { 0:NaN,1:NaN,2:NaN,3:NaN };
				var meds = { 0:NaN,1:NaN,2:NaN,3:NaN };
				var axna = { 0:"Temp", 1:"Humi", 2:"TempExt", 3:"HumiExt" };
				var show = { 0:dom.byId("show-temp").checked,
							 1:dom.byId("show-humi").checked,
							 2:dom.byId("show-ext-temp").checked,
							 3:dom.byId("show-ext-humi").checked };
				var list = { 0:[], 1:[], 2:[], 3:[] };
                for (var time in hst.data){
					for ( var n in show ){
						var val = hst.data[time][n];
						if ( show[n] )
							list[n].push( {x:time, y:val } ); 
						if ( isNaN(mins[n]) || (mins[n] > val) )
							mins[n] = val;
						if ( isNaN(maxs[n]) || (maxs[n] < val) )
							maxs[n] = val;
						if ( isNaN(meds[n]) ){
							meds[n] = val;
						} else {
							meds[n] = (meds[n]+val)/2;
						}
					}
                }
				html.set(dom.byId("history-stats-te"), "T(int): " + mins[0].toFixed(1) + " ...(" + meds[0].toFixed(1) + ")... " + maxs[0].toFixed(1) + "" );
				html.set(dom.byId("history-stats-ex"), "T(est): " + mins[2].toFixed(1) + " ...(" + meds[2].toFixed(1) + ")... " + maxs[2].toFixed(1) + "" );
				html.set(dom.byId("history-stats-hu"), "H(int): " + mins[1].toFixed(1) + " ...(" + meds[1].toFixed(1) + ")... " + maxs[1].toFixed(1) + "" );
				html.set(dom.byId("history-stats-hx"), "H(est): " + mins[3].toFixed(1) + " ...(" + meds[3].toFixed(1) + ")... " + maxs[3].toFixed(1) + "" );
				for ( var n in list )
					hst.grp.updateSeries( axna[n], list[n] );                
        		hst.grp.render();        
			},
			
            clearData: function(){
                hst.data = {};
                hst.drawGraph();                                
            },
        
            setData: function(new_rows){
                var len = new_rows.length;
                var pos = 0;
                while ( pos < len ){
                    var line_end = new_rows.indexOf( "\n", pos );
					if ( line_end != -1 ){
						var row = new_rows.substr( pos, line_end-pos+1 );
						var split_row = row.split(" ");
						hst.data[ parseInt(split_row[0]) ] = { 0: parseFloat(split_row[1]), 1: parseFloat(split_row[2]), 
															   2: parseFloat(split_row[3]), 3: parseFloat(split_row[3]) };
						pos = line_end+1;
					} else {
						pos = len;
					 }
                }
                hst.drawGraph();
            },
        
 			update: function(){
				if ( hst.timer ){
                    window.clearTimeout( hst.timer );
                    hst.timer = null;
				}
                if ( !historyUseRange.checked ){
                    hst.hStart = new Date();
                    hst.hEnd = new Date();
                    historyEnd.set("value", hst.hStart );
                    historyStart.set("value", hst.hEnd );
                } else {
                    hst.hEnd = historyEnd.get("value")
                    hst.hStart = historyStart.get("value")
                }
                var sDate = hst.hStart;sDate.setHours(0);sDate.setMinutes(0);sDate.setSeconds(0);
                var eDate = hst.hEnd;eDate.setHours(23);eDate.setMinutes(59);eDate.setSeconds(59);
				var startDate = Math.floor( sDate / 1000 );
				var endDate = Math.floor( eDate / 1000 ); 

                hst.clearData();                
                postRequest("cgi-bin/history",startDate+":"+endDate+":200",
                    function(result){
                        if ( result )
                            hst.setData(result);
						if ( !historyUseRange.checked )
							hst.timer = window.setTimeout( function(){ hst.update(); }, 60 * 1000 );
                    },
                    function(err){
						hst.timer = window.setTimeout( function(){ hst.update(); }, 60 * 1000 );
					});                    
			}
		};
    
    
	hst.grp.setTheme(Chris);
    
	hst.grp.addPlot("tempPlot",{
                type: Lines,lines: true, areas: false, markers: false,
                tension: "X",
                stroke: {color: "red",  width: 1}
            });
	hst.grp.addAxis("x", 	{
                plot:"tempPlot", 
                majorTicks: true, majorLabels: true,
                minorTicks: false,minorLabels: false,
                microTicks: false,
                labelFunc:function(text,value,prec){
                        return new Date(parseInt(value)*1000).toLocaleTimeString();
                }
            });
	hst.grp.addAxis("y", 	{
                plot:"tempPlot", 
                vertical: true, 
                dropLabels: false,
                majorTickStep: 5, majorTicks: true, majorLabels: true,
                minorTickStep: 1, minorTicks: true, minorLabels: false,
                microTickStep: 0.1, microTicks: false,
                fixLower: "major",  fixUpper: "major"
            });
	hst.grp.addSeries("Temp", [],{ plot: "tempPlot"});
	hst.grp.addSeries("TempExt", [],{ plot: "tempPlot", stroke: {color:"blue"} });
    
	hst.grp.addPlot("humiPlot",{
                type: Lines,lines: true, areas: false, markers: false,
                tension: "X",
                hAxis: "x",vAxis: "h",
                stroke: {color: "yellow", width: 1	}
            });
	hst.grp.addAxis("h", 	{
                plot:"humiPlot", 
                vertical: true, leftBottom: false,
                majorTickStep: 5, 
                minorTickStep: 1,
                fixLower: "major", fixUpper: "major"
            });
	hst.grp.addSeries("Humi",[],{plot: "humiPlot"});
	hst.grp.addSeries("HumiExt",[],{plot: "humiPlot", stroke: { color: "violet"} });

	on(dom.byId("history-size"),"click", function(v) {
		dclass.toggle(dom.byId("history-graph"), "history-big");
		hst.grp.resize();
	});

});
