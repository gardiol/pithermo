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
			timer: null,
    		data:  {},
        
            grp: new Chart("history-graph",{ title: "Storico", titlePos: "bottom", titleGap: 25}),

     		xrefExtTData: {},
    		xrefExtHData: {},
                
        
        
            toggleRange: function(){
                dclass.toggle(dom.byId("history-range"), "celated");
                hst.update();
            },
        
        
/*			setData: function(){
				var mins = { t_i:200, t_e: 200, h_i: 200, h_e: 200 };
				var maxs = { t_i:-200, t_e:-200, h_i:-200, h_e:-200 };
				var avgs = { t_i:0, t_e: 0, h_i: 0, h_e: 0 };
        		hst.xrefExtTData = {};
        		hst.xrefExtHData = {};
        		var t = [], h = [], x = [], y = [];
                var t_m = [];
                
        		var s = hst.sel.get("value"); ??
                
        		var show_h = hst.hck.get("checked");
        		if ( hst.data ){
            	var te = [], ex = [], hu = [], ti = [], hx = [];
            	for ( var v = 0; (v < s) || (ti.length < 15); v++ ){
               	if ( hst.data[v] ){
                  	te = hst.data[v].te.concat(te);
                  	ex = hst.data[v].e_te.concat(ex);
                  	hu = hst.data[v].hu.concat(hu);
                  	hx = hst.data[v].e_hu.concat(hx);
                  	ti = hst.data[v].ti.concat(ti);
                	} else {
                  	break;
               	}
            	}
            	var n_pts = ti.length;
                var avg_t = null;
            	for ( p = 0; p < n_pts; p++ ){
                    if ( !avg_t )
                        avg_t = (avg_t + te[p]) / 2;
                    else
                        avg_t = te[p];
						if ( mins.t_i > te[p] ) mins.t_i = te[p];
						if ( mins.t_e > ex[p] ) mins.t_e = ex[p];
						if ( maxs.t_i < te[p] ) maxs.t_i = te[p];
						if ( maxs.t_e < ex[p] ) maxs.t_e = ex[p];
						avgs.t_i = ((avgs.t_i+te[p])/2);
						avgs.t_e = ((avgs.t_e+ex[p])/2);
                	t.push( {x:ti[p], y:te[p] } );
                    t_m.push( {x:ti[p], y: avg_t } );
                	if ( show_h ){
							if ( mins.h_i > hu[p] ) mins.h_i = hu[p];
							if ( mins.h_e > hx[p] ) mins.h_e = hx[p];
							if ( maxs.h_i < hu[p] ) maxs.h_i = hu[p];
							if ( maxs.h_e < hx[p] ) maxs.h_e = hx[p];
							avgs.h_i = ((avgs.h_i+hu[p])/2);
							avgs.h_e = ((avgs.h_e+hx[p])/2);
               	   h.push( {x:ti[p], y:hu[p] } );
                    	y.push( {x:ti[p], y:hx[p] } );
               	}
                	x.push( {x:ti[p], y:ex[p] } );
                	hst.xrefExtTData[ti[p]] = ex[p];
                	hst.xrefExtHData[ti[p]] = hx[p];
            	}
        		}
        		hst.grp.updateSeries("Temperatura", t );
        		hst.grp.updateSeries("TemperaturaMed", t_m );
        		hst.grp.updateSeries("Esterna", x );
        		hst.grp.updateSeries("Umidita", h );
        		hst.grp.updateSeries("EsternaUmidita", y );
        		var ts = 60; // 1 min
        		if ( t.length > 0 ){
         	   var d = t[ t.length-1 ].x - t[0].x;
            	ts = Math.floor( ( d / Math.min(10, t.length) ) / (15*60) +1 ) * 15*60;
        		}
        		hst.grp.getAxis("x").opt.majorTickStep = ts;
        		hst.grp.render();        
				html.set(dom.byId("history-stats-te"), "T(int): " + mins.t_i + " ...(" + avgs.t_i.toFixed(1) + ")... " + maxs.t_i + "" );
				html.set(dom.byId("history-stats-ex"), "T(est): " + mins.t_e + " ...(" + avgs.t_e.toFixed(1) + ")... " + maxs.t_e + "" );
				if ( show_h ){
					html.set(dom.byId("history-stats-hu"), "H(int): " + mins.h_i + " ...(" + avgs.h_i.toFixed(1) + ")... " + maxs.h_i + "" );
					html.set(dom.byId("history-stats-hx"), "H(est): " + mins.h_e + " ...(" + avgs.h_e.toFixed(1) + ")... " + maxs.h_e + "" );
					dclass.remove( dom.byId("history-stats-h"), "hidden" );
				} else {
					html.set(dom.byId("history-stats-hu"), "");
					html.set(dom.byId("history-stats-hx"), "");
					dclass.add( dom.byId("history-stats-h"), "hidden" );
				}
    		},
			disable: function(){
				hst.data = null;
            hst.grp.updateSeries("Temperatura", [] );
            hst.grp.updateSeries("TemperaturaMed", [] );
            hst.grp.updateSeries("Esterna", [] );
            hst.grp.updateSeries("Umidita", [] );
            hst.grp.updateSeries("EsternaUmidita", []);
            hst.grp.render();
			},*/
        

            drawGraph: function(){
                var show_t = dom.byId("show-temp").checked;
                var show_h = dom.byId("show-humi").checked;
                var show_et = dom.byId("show-ext-temp").checked;
                var show_eh = dom.byId("show-ext-humi").checked;
                var serie_t = [];
                var serie_h = [];
                var serie_et = [];
                var serie_eh = [];
                for (var time in hst.data){
                    if ( show_t )
                        serie_t.push( {x:time, y: hst.data[time][0] } );
                    if ( show_h )
                        serie_h.push( {x:time, y: hst.data[time][1] } );
                    if ( show_et )
                        serie_et.push( {x:time, y: hst.data[time][2] } );
                    if ( show_eh )
                        serie_eh.push( {x:time, y: hst.data[time][3] } );
                }
                hst.grp.updateSeries("Temp", serie_t );                
        		hst.grp.updateSeries("TempExt", serie_et );
        		hst.grp.updateSeries("Humi", serie_h );
        		hst.grp.updateSeries("HumiExt", serie_eh );
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
					var now = new Date();
					var start = new Date();
					start.setHours(0);
					start.setMinutes(0);
					start.setSeconds(0);
                    historyEnd.set("value", now );
                    historyStart.set("value", start );
                }
				var endDate = Math.floor( historyEnd.get("value") / 1000 );
				var startDate = Math.floor( historyStart.get("value") / 1000 ); 

                hst.clearData();                
                postRequest("cgi-bin/history",startDate+":"+endDate+":200",
                    function(result){
                        if ( result )
                            hst.setData(result);
						if ( !historyUseRange.checked )
							hst.timer = window.setTimeout( function(){ hst.update(); }, 60 * 1000 );
                    },
                    function(err){
                        hst.disable();
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

/*    new MouseIndicator(hst.grp, "humiPlot",{ 
                series: "Umidita", start: true, mouseOver: true,
                labelFunc: function(v){
                    if ( v.y && v.x )
                        return "H: "+v.y.toFixed(1)+"/"+ ((hst.xrefExtHData[v.x] != null) ? hst.xrefExtHData[v.x].toFixed(1) : "-");
                    else
                        return "";
                }
                });
	new MouseIndicator(hst.grp, "tempPlot",{ 
                series: "Temperatura",mouseOver: true,
                labelFunc: function(v){
                    if ( v.y && v.x )
                        return "T: "+v.y.toFixed(1)+"/" + ((hst.xrefExtTData[v.x] != null) ? hst.xrefExtTData[v.x].toFixed(1) : "-") +" (" + (new Date(v.x*1000).toLocaleString())+")";
                    else
                        return "";
                }
                });*/

	on(dom.byId("history-size"),"click", function(v) {
		dclass.toggle(dom.byId("history-graph"), "history-big");
		hst.grp.resize();
	});

});
