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
    "dojox/charting/themes/Chris",
    "dojox/charting/plot2d/Areas",
    "dojox/charting/plot2d/Markers",
    "dojox/charting/action2d/Tooltip",
    "dojox/charting/action2d/Magnify",
    "dojox/charting/widget/Legend",
    "dojo/domReady!"], 
function( dom, attr, dclass, style, html, on,// Dojo
          CheckBox, Select, DateTextBox, DijitTooltip,// Dijit
          Chart, Default, Lines, Chris, Areas, Markers, Tooltip, Magnify, Legend )// Charing
{
    hst = { 
        ttipRect: null,
        hStart: null,
        hEnd: null,
        timer: null,
        data: {},
        mins: null,
        maxs: null,
        avgs: null,
        
        grp: new Chart("history-graph",{ title: "Storico", titlePos: "bottom", titleGap: 25}),

        toggleRange: function(){
            dclass.toggle(dom.byId("history-range"), "hidden");
            hst.update();
        },
        
        drawGraph: function(){
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
                        list[n].push( {x:time, y:val.toFixed(1) } ); 
                }
            }
            if ( hst.mins ){
                html.set(dom.byId("history-stats-te"), "T(int): " + hst.mins[0].toFixed(1) + " ...(" + hst.avgs[0].toFixed(1) + ")... " + hst.maxs[0].toFixed(1) + "" );
                html.set(dom.byId("history-stats-ex"), "T(est): " + hst.mins[2].toFixed(1) + " ...(" + hst.avgs[2].toFixed(1) + ")... " + hst.maxs[2].toFixed(1) + "" );
                html.set(dom.byId("history-stats-hu"), "H(int): " + hst.mins[1].toFixed(1) + " ...(" + hst.avgs[1].toFixed(1) + ")... " + hst.maxs[1].toFixed(1) + "" );
                html.set(dom.byId("history-stats-hx"), "H(est): " + hst.mins[3].toFixed(1) + " ...(" + hst.avgs[3].toFixed(1) + ")... " + hst.maxs[3].toFixed(1) + "" );
            }
            for ( var n in list )
                hst.grp.updateSeries( axna[n], list[n] );                
            hst.grp.render();        
        },
			
        clearData: function(){
            hst.data = {};
            hst.mins = null;
            hst.maxs = null;
            hst.avgs = null;
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
                                                                    2: parseFloat(split_row[3]), 3: parseFloat(split_row[4]) };
                    pos = line_end+1;
                } else {
                    var row = new_rows.substr( pos );
                    var split_row = row.split(" ");
                    hst.mins = {0: parseFloat(split_row[0]), 1: parseFloat(split_row[3]), 2:parseFloat(split_row[6]), 3:parseFloat(split_row[9]) };
                    hst.maxs = {0: parseFloat(split_row[1]), 1: parseFloat(split_row[4]), 2:parseFloat(split_row[7]), 3:parseFloat(split_row[10]) };
                    hst.avgs = {0: parseFloat(split_row[2]), 1: parseFloat(split_row[5]), 2:parseFloat(split_row[8]), 3:parseFloat(split_row[11]) };                    
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
                historyEnd.set("value", hst.hEnd );
                historyStart.set("value", hst.hStart );
            } else {
                hst.hEnd = historyEnd.get("value")
                hst.hStart = historyStart.get("value")
            }
            var sDate = hst.hStart;sDate.setHours(0);sDate.setMinutes(0);sDate.setSeconds(0);
            var eDate = hst.hEnd;eDate.setHours(23);eDate.setMinutes(59);eDate.setSeconds(59);
            var startDate = Math.floor( sDate / 1000 );
            var endDate = Math.floor( eDate / 1000 ); 
            var n_samples = (endDate-startDate)/300;
            if ( n_samples < 2 ) n_samples = 2;
            if ( n_samples > 60 ) n_samples = 60;
            
            hst.clearData();
            postRequest("cgi-bin/history",startDate+":"+endDate+":"+n_samples.toFixed(0),
                function(result){
                    if ( result )
                        hst.setData(result);
                    if ( !historyUseRange.checked )
                        hst.timer = window.setTimeout( function(){ hst.update(); }, 60 * 1000 );
                },
                function(err){
                    hst.timer = window.setTimeout( function(){ hst.update(); }, 60 * 1000 );
                });                    
        },
    
        startup:function(){
            hst.grp.setTheme(Chris);
            
            hst.grp.addPlot("tempPlot",{
                        type: Lines,lines: true, areas: false, markers: true,
                        tension: "X",
                        stroke: {color: "red",  width: 1}
                    });
            hst.grp.addAxis("x", 	{
                        plot:"tempPlot", 
                        majorTicks: true, majorLabels: true,
                        minorTicks: false,minorLabels: false,
                        microTicks: false,
                        labelFunc:function(text,value,prec){
                            return utils.printDate(value*1000);
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
            hst.grp.addSeries("Temp", [{x:0,y:0},{x:1,y:1}],{ plot: "tempPlot", legend:"Temperatura interna"});
            hst.grp.addSeries("TempExt", [{x:0,y:0},{x:1,y:1}],{ plot: "tempPlot", legend:"Temperatura esterna", stroke: {color:"blue"} });
            
            hst.grp.addPlot("humiPlot",{
                        type: Lines,lines: true, areas: false, markers: true,
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
            hst.grp.addSeries("Humi",[{x:0,y:0},{x:1,y:1}],{plot: "humiPlot", legend:"Umidità interna"});
            hst.grp.addSeries("HumiExt",[{x:0,y:0},{x:1,y:1}],{plot: "humiPlot", legend: "Umidità esterna", stroke: { color: "violet"} });

            new Magnify( hst.grp, "tempPlot", { scale: 4 });
            new Magnify( hst.grp, "humiPlot", { scale: 4 });

            
            hst.grp.connectToPlot("tempPlot",
                function(evt){
                    if ( evt.type == "onclick" ){
                            var lt = hst.grp.getCoords();
                            hst.ttipRect = {type: "rect"};
                            hst.ttipRect.x = Math.round(evt.cx + lt.x);
                            hst.ttipRect.y = Math.round(evt.cy + lt.y);
                            hst.ttipRect.w = hst.ttipRect.h = 1;    
                            DijitTooltip.show("<div style='text-align:center;'>"+evt.y+"<br><span style='font-size:60%;'>" + utils.date2str(evt.x*1000) + "" + utils.time2str(evt.x*1000)+"</span></div>", hst.ttipRect,["after-centered", "before-centered"]);
                    } else if(evt.type === "onplotreset" || evt.type === "onmouseout"){
                            if ( hst.ttipRect != null ){
                                DijitTooltip.hide(hst.ttipRect);
                                hst.ttipRect = null;
                            }
                    }
                });
            
            hst.grp.render();        
            new Legend({chartRef:hst.grp}, 'history-legend');

            on(dom.byId("history-size"),"click", function(v) {
                dclass.toggle(dom.byId("history-graph"), "history-big");
                hst.grp.resize();
            });
            hst.update();
        }
    };
});
