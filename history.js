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
    "dojox/charting/plot2d/Columns",
    "dojox/charting/plot2d/Lines",
    "dojox/charting/plot2d/Grid",
    "dojox/charting/themes/Chris",
    "dojox/charting/plot2d/Areas",
    "dojox/charting/plot2d/Markers",
    "dojox/charting/action2d/Tooltip",
    "dojox/charting/action2d/Magnify",
    "dojox/charting/widget/Legend",
    "dojo/domReady!"], 
function( dom, attr, dclass, style, html, on,// Dojo
          CheckBox, Select, DateTextBox, DijitTooltip,// Dijit
          Chart, Default, Columns, Lines, Grid, Chris, Areas, Markers, Tooltip, Magnify, Legend )// Charing
{
    hst = { 
        ttipRect: null,
        data: {},
        mins: null,
        maxs: null,
        avgs: null,
        xref: [],
        nPoints:0,
        xScale: 1,
        xOffset: 0,
        
        grp: new Chart("history-graph",{ title: "Storico", titlePos: "bottom", titleGap: 25}),

        fuzzle:function(o,s){
            hst.xScale = Math.min( hst.nPoints/60, Math.max( 1, hst.xScale * s ) );
            var step = (hst.nPoints/60)*hst.xScale;
            hst.xOffset = Math.min( hst.nPoints-60, Math.max( 0, hst.xOffset + o * step) );            
            hst.grp.setAxisWindow( "x", hst.xScale, hst.xOffset );
            hst.grp.render(); 
        },

        toggleRange: function(){
            dclass.toggle(dom.byId("history-range"), "hidden");
            if ( !historyUseRange.checked )
                hst.update();
        },
        
        drawGraph: function(){
            var axna = { 0:"Temp", 1:"Humi", 2:"TempExt", 3:"HumiExt", 4:"PelletOn", 5:"PelletOff", 6:"GasOn", 7:"GasOff" };
            var show = { 0:dom.byId("show-temp").checked,
                         1:dom.byId("show-humi").checked,
                         2:dom.byId("show-ext-temp").checked,
                         3:dom.byId("show-ext-humi").checked, 4:true, 5:true, 6:true, 7:true };
            var list = { 0:[], 1:[], 2:[], 3:[], 4:[], 5:[], 6:[], 7:[] };
            hst.nPoints = 0;
            for (var time in hst.data){
                hst.xref.push( time );
                for ( var n in axna ){
                    var val = hst.data[time][n];
                    if ( show[n] )
                        list[n].push( parseFloat(val.toFixed(1)) );
                }
                hst.nPoints++;
            }
            if ( hst.mins ){
                html.set(dom.byId("history-stats-te"), hst.mins[0].toFixed(1) + " (" + hst.avgs[0].toFixed(1) + ") " + hst.maxs[0].toFixed(1) );
                html.set(dom.byId("history-stats-ex"), hst.mins[2].toFixed(1) + " (" + hst.avgs[2].toFixed(1) + ") " + hst.maxs[2].toFixed(1) );
                html.set(dom.byId("history-stats-hu"), hst.mins[1].toFixed(1) + " (" + hst.avgs[1].toFixed(1) + ") " + hst.maxs[1].toFixed(1) );
                html.set(dom.byId("history-stats-hx"), hst.mins[3].toFixed(1) + " (" + hst.avgs[3].toFixed(1) + ") " + hst.maxs[3].toFixed(1) );
            }
            for ( var n in axna )
                hst.grp.updateSeries( axna[n], list[n] );    

            hst.grp.setAxisWindow("x", hst.xScale, hst.xOffset ); 
            hst.grp.render();        
        },
			    
        setData: function(new_rows){
            var len = new_rows.length;
            var pos = 0;
            while ( pos < len ){
                var line_end = new_rows.indexOf( "\n", pos );
                if ( line_end != -1 ){
                    var row = new_rows.substr( pos, line_end-pos+1 );
                    var split_row = row.split(" ");
                    hst.data[ parseInt(split_row[0]) ] = { 0: parseFloat(split_row[1]), 
                                                           1: parseFloat(split_row[2]), 
                                                           2: parseFloat(split_row[3]), 
                                                           3: parseFloat(split_row[4]), 
                                                           4: parseFloat(split_row[5]), 
                                                           5: parseFloat(split_row[6]), 
                                                           6: parseFloat(split_row[7]), 
                                                           7: parseFloat(split_row[8]) };
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
            if ( !historyUseRange.checked ){
                historyEnd.set("value", new Date() );
                historyStart.set("value", new Date() );
            }
            var sDate = historyStart.get("value");sDate.setHours(0);sDate.setMinutes(0);sDate.setSeconds(0);
            var eDate = historyEnd.get("value");eDate.setHours(23);eDate.setMinutes(59);eDate.setSeconds(59);
            var startDate = Math.floor( sDate / 1000 );
            var endDate = Math.floor( eDate / 1000 ); 
            var n_samples = (endDate-startDate)/600; // 1pt every 10 mins
            if ( n_samples < 2 ) n_samples = 2;
            if ( n_samples > 6*48 ) n_samples = 6*48; 
            
            postRequest("cgi-bin/history",startDate+":"+endDate+":"+n_samples.toFixed(0),
                function(result){
                    if ( result ){
                        hst.data = {};
                        hst.mins = null;
                        hst.maxs = null;
                        hst.avgs = null;
                        hst.xref = [];
                        hst.nPoints = 0;
                        hst.setData(result);
                    }
                },
                function(err){
                });                    
        },
        showTip:function(r,t,p){
                var lt = hst.grp.getCoords();
                hst.ttipRect = {type: "rect"};
                hst.ttipRect.x = Math.round(r.x + lt.x);
                hst.ttipRect.y = Math.round(r.y + lt.y);
                hst.ttipRect.w = hst.ttipRect.h = 1;    
                DijitTooltip.show(t, hst.ttipRect,p);
        },
        hideTip:function(){
                if ( hst.ttipRect != null ){
                    DijitTooltip.hide(hst.ttipRect);
                    hst.ttipRect = null;
                }
        },

        startup:function(){
            hst.grp.setTheme(Chris);
            
            hst.grp.addPlot("tempPlot",{tension: "X", type: Lines,lines: true, areas: false, markers: false, hAxis:"x", vAxis:"t"});
            hst.grp.addAxis("x", 	{plot:"tempPlot",
                        majorTickStep: 6, majorTicks: true, majorLabels: true,
                        minorTicks: true, minorLabels: true,
                        microTicks: false,microLabels: false,
                        labelFunc:function(text,value,prec){
                            return utils.date2str( hst.xref[value]*1000)+"<br> "+utils.time2str( hst.xref[value]*1000);
                        }
                    });
            hst.grp.addAxis("t", 	{plot:"tempPlot", 
                        vertical: true, 
                        dropLabels: false,
                        majorTickStep: 5, majorTicks: true, majorLabels: true,
                        minorTickStep: 1, minorTicks: true, minorLabels: false,
                        microTickStep: 0.1, microTicks: false,
                        fixLower: "major",  fixUpper: "major"
                    });
            hst.grp.addSeries("Temp", [0,1],{ plot: "tempPlot", legend:"Temperatura interna"});
            hst.grp.addSeries("TempExt", [0,1],{ plot: "tempPlot", legend:"Temperatura esterna"});
            new Magnify( hst.grp, "tempPlot", { scale: 3 });
            hst.grp.connectToPlot("tempPlot",
                function(evt){
                    if ( evt.type == "onclick" ){
                        hst.showTip( {x:evt.cx,y:evt.cy}, 
                                     "<div style='text-align:center;'>"+evt.y+"°C<br><span style='font-size:60%;'>" + utils.printDate(hst.xref[evt.x]*1000) +"</span></div>",
                                     ["after-centered", "before-centered"] );
                    } else if(evt.type === "onplotreset" || evt.type === "onmouseout"){
                        hst.hideTip();
                    }
                });
            
            hst.grp.addPlot("humiPlot",{ type: Lines,lines: true, areas: false, markers: false, tension: "X", hAxis: "x",vAxis: "h" });
            hst.grp.addAxis("h", {plot:"humiPlot", vertical: true, leftBottom: false,
                                    majorTickStep: 10, majorTicks: true, majorLabels: true,
                                    minorTickStep: 1, minorTicks: true, minorLabels: false,
                                    microTicks: false, microLabels: false,
                                    fixLower: "major", fixUpper: "major"});
            hst.grp.addSeries("Humi",[0,100],{plot: "humiPlot", legend:"Umidità interna"});
            hst.grp.addSeries("HumiExt",[0,100],{plot: "humiPlot", legend: "Umidità esterna"});
            new Magnify( hst.grp, "humiPlot", { scale: 3 });
            hst.grp.connectToPlot("humiPlot",
                function(evt){
                    if ( evt.type == "onclick" ){
                        hst.showTip( {x:evt.cx,y:evt.cy}, 
                                     "<div style='text-align:center;'>"+evt.y+"%<br><span style='font-size:60%;'>" + utils.printDate(hst.xref[evt.x]*1000) +"</span></div>",
                                     ["after-centered", "before-centered"] );
                    } else if(evt.type === "onplotreset" || evt.type === "onmouseout"){
                        hst.hideTip();
                    }
                });

            hst.grp.addPlot("eventsPlot",{ type: Columns, hAxis: "x", vAxis:"e" });
            hst.grp.addAxis("e", {plot:"eventsPlot", vertical: true, min: 0, max: 1,
                                majorTicks: false, majorLabels: false,
                                minorTicks: false, minorLabels: false,
                                microTicks: false, microLabels: false });
            hst.grp.addSeries("GasOn",[0,1],{plot: "eventsPlot", legend:"Gas ON"});
            hst.grp.addSeries("GasOff",[0,1],{plot: "eventsPlot", legend:"Gas OFF"});
            hst.grp.addSeries("PelletOn",[0,1],{plot: "eventsPlot", legend:"Pellet ON"});
            hst.grp.addSeries("PelletOff",[0,1],{plot: "eventsPlot", legend:"Pellet OFF"});
            hst.grp.connectToPlot("eventsPlot",
                function(evt){
                    if ( evt.type == "onclick" ){
                        hst.showTip( evt.shape.shape, 
                                     "<div style='text-align:center;'>"+evt.run.name+"<br><span style='font-size:60%;'>"+utils.printDate(hst.xref[evt.x]*1000) +"</span></div>", ["above"] );
                    } else if(evt.type === "onplotreset" || evt.type === "onmouseout"){
                        hst.hideTip();
                    }
                });
            
            hst.grp.addPlot("gridPlot", { type: Grid, hAxis: "x",vAxis: "t",
                    hMajorLines: true,
                    hMinorLines: true,
                    vMajorLines: false,
                    vMinorLines: false,
                    majorHLine: { color: "gray", width: 1, style: "dash"},
                    minorHLine: { color: "gray", width: 1, style: "dot" } });
            
            hst.grp.render();        
            new Legend({chartRef:hst.grp, horizontal:4}, 'history-legend');

            on(dom.byId("history-size"),"click", function(v) {
                dclass.toggle(dom.byId("history-graph"), "history-big");
                hst.grp.resize();
            });
            
            hst.update();
        }
                
    };
});
