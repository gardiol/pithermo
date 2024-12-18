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
        axNames: { 0:"Temp", 1:"Humi", 2:"TempExt", 3:"HumiExt", 4:"PelletOn", 5:"PelletOff", 6:"GasOn", 7:"GasOff" },
        data: { 0:[20,20,20], 1:[100,100,100], 2:[20,20,20], 3:[100,100,100], 4:[0,0,0], 5:[0,0,0], 6:[0,0,0], 7:[0,0,0] },
        timeRef: [0,1,2],
        xScale: 1,
        xOffset: 0,
        ttipRect: null,
        
        grp: new Chart("history-graph",{ title: "Storico", titlePos: "bottom", titleGap: 25}),

        fuzzle:function(o,s){
            var nPoints = hst.data[0].length;
            hst.xScale = Math.min( nPoints/60, Math.max( 1, hst.xScale * s ) );
            var step = (nPoints/60)*hst.xScale;
            hst.xOffset = Math.min( nPoints-60, Math.max( 0, hst.xOffset + o * step) );            
            hst.grp.setAxisWindow( "x", hst.xScale, hst.xOffset );
            hst.grp.render(); 
        },

        toggleRange: function(){
            dclass.toggle(dom.byId("history-range"), "hidden");
            if ( !historyUseRange.checked )
                hst.update();
        },
        
        drawSerie: function(s,v){            
            hst.grp.updateSeries( hst.axNames[s], v ? hst.data[s] : [] );    
            hst.grp.render();        
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
            
            postRequest("cgi-bin/history-cgi",startDate+":"+endDate+":"+n_samples.toFixed(0),
                function(new_rows){
                    if ( new_rows ){
                        var len = new_rows.length;
                        var pos = 0;
                        hst.data = { 0:[], 1:[], 2:[], 3:[], 4:[], 5:[], 6:[], 7:[] };
                        hst.timeRef = [];
                        while ( pos < len ){
                            var line_end = new_rows.indexOf( "\n", pos );
                            if ( line_end != -1 ){
                                var row = new_rows.substr( pos, line_end-pos+1 );
                                var split_row = row.split(" ");
                                var time = parseInt(split_row[0]);
                                for ( var q = 0; q < 8; q++ )
                                    hst.data[q].push( parseFloat(split_row[q+1]) );
                                hst.timeRef.push( time );
                                pos = line_end+1;
                            } else {
                                var row = new_rows.substr( pos );
                                var split_row = row.split(" ");
                                html.set(dom.byId("history-stats-te-min"), parseFloat(split_row[0]).toFixed(1) );
                                html.set(dom.byId("history-stats-te-avg"), parseFloat(split_row[2]).toFixed(1) );
                                html.set(dom.byId("history-stats-te-max"), parseFloat(split_row[1]).toFixed(1) );
                                html.set(dom.byId("history-stats-ex-min"), parseFloat(split_row[6]).toFixed(1) );
                                html.set(dom.byId("history-stats-ex-avg"), parseFloat(split_row[8]).toFixed(1) );
                                html.set(dom.byId("history-stats-ex-max"), parseFloat(split_row[7]).toFixed(1) );
                                html.set(dom.byId("history-stats-hu-min"), parseFloat(split_row[3]).toFixed(1) );
                                html.set(dom.byId("history-stats-hu-avg"), parseFloat(split_row[5]).toFixed(1) );
                                html.set(dom.byId("history-stats-hu-max"), parseFloat(split_row[4]).toFixed(1) );
                                html.set(dom.byId("history-stats-hx-min"), parseFloat(split_row[9]).toFixed(1) );
                                html.set(dom.byId("history-stats-hx-avg"), parseFloat(split_row[11]).toFixed(1) );
                                html.set(dom.byId("history-stats-hx-max"), parseFloat(split_row[10]).toFixed(1) );
                                pos = len;
                            }
                        }
                        hst.grp.updateSeries( hst.axNames[0], show_temp.checked ? hst.data[0] : [] );
                        hst.grp.updateSeries( hst.axNames[1], show_humi.checked ? hst.data[1] : [] );
                        hst.grp.updateSeries( hst.axNames[2], show_ext_temp.checked ? hst.data[2] : [] );
                        hst.grp.updateSeries( hst.axNames[3], show_ext_humi.checked ? hst.data[3] : [] );
                        hst.grp.updateSeries( hst.axNames[4], show_pellet.checked ? hst.data[4] : [] );
                        hst.grp.updateSeries( hst.axNames[5], show_pellet.checked ? hst.data[5] : [] );
                        hst.grp.updateSeries( hst.axNames[6], show_gas.checked ? hst.data[6] : [] );
                        hst.grp.updateSeries( hst.axNames[7], show_gas.checked ? hst.data[7] : [] );
                        hst.grp.render(); 

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
            
            hst.grp.addPlot("tempPlot",
                            {
                                tension: "X", 
                                type: Lines,
                                lines: true, 
                                areas: false, 
                                markers: false, 
                                hAxis:"x",
                                 vAxis:"t"
                             });

            hst.grp.addAxis("x", 	
                            {
                                plot:"tempPlot",
                                majorTickStep: 6, 
                                majorTicks: true, 
                                majorLabels: true,
                                minorTicks: true, 
                                minorLabels: true,
                                microTicks: false,
                                microLabels: false,
                                labelFunc:function(text,value,prec){
                                    return utils.date2str( hst.timeRef[value]*1000)+"<br> "+utils.time2str( hst.timeRef[value]*1000);
                                }
                            });
            hst.grp.addAxis("t", 	
                            {
                                plot:"tempPlot", 
                                vertical: true, 
                                dropLabels: false,
                                majorTickStep: 5, 
                                majorTicks: true, 
                                majorLabels: true,
                                minorTickStep: 1, 
                                minorTicks: true, 
                                minorLabels: false,
                                microTickStep: 0.1, 
                                microTicks: false,
                                fixLower: "major",  
                                ixUpper: "major"
                            });

            hst.grp.addSeries("Temp", 
                                hst.data[0],
                                { 
                                    plot: "tempPlot", 
                                    legend:"Temperatura interna"
                                });
            hst.grp.addSeries("TempExt", 
                                hst.data[1],
                                { 
                                    plot: "tempPlot", 
                                    legend:"Temperatura esterna"
                                });

            hst.grp.addPlot("humiPlot",
                            { 
                                type: Lines,
                                lines: true, 
                                areas: false, 
                                markers: false, 
                                tension: "X",
                                hAxis: "x",
                                vAxis: "h" 
                            });

            hst.grp.addAxis("h",
                            {
                                plot:"humiPlot", 
                                vertical: true, 
                                leftBottom: false,
                                majorTickStep: 10, 
                                majorTicks: true, 
                                majorLabels: true,
                                minorTickStep: 1, 
                                minorTicks: true, 
                                minorLabels: false,
                                microTicks: false, 
                                microLabels: false,
                                fixLower: "major", 
                                fixUpper: "major"
                            });

            hst.grp.addSeries("Humi",
                                hst.data[2],
                                {
                                    plot: "humiPlot", 
                                    legend:"Umidità interna"
                                });

            hst.grp.addSeries("HumiExt",
                                hst.data[3],
                                {
                                    plot: "humiPlot", 
                                    legend: "Umidità esterna"
                                });

            hst.grp.addPlot("eventsPlot",
                            {
                                type: Columns, 
                                hAxis: "x", 
                                vAxis:"e"
                            });

            hst.grp.addAxis("e", 
                            {
                                plot:"eventsPlot", 
                                vertical: true, 
                                min: 0, 
                                max: 1,
                                majorTicks: false, 
                                majorLabels: false,
                                minorTicks: false, 
                                minorLabels: false,
                                microTicks: false, 
                                microLabels: false
                            });

            hst.grp.addSeries("GasOn",
                                hst.data[6],
                                {
                                    plot: "eventsPlot",
                                    legend:"Gas ON"
                                });

            hst.grp.addSeries("GasOff",
                                hst.data[7],
                                {
                                    plot: "eventsPlot", 
                                    legend:"Gas OFF"
                                });

            hst.grp.addSeries("PelletOn",
                                hst.data[4],
                                {
                                    plot: "eventsPlot", 
                                    legend:"Pellet ON"
                                });

            hst.grp.addSeries("PelletOff",
                                hst.data[5],
                                {
                                    plot: "eventsPlot", 
                                    legend:"Pellet OFF"
                                });
            
            hst.grp.addPlot("gridPlot", 
                            { 
                                type: Grid, 
                                hAxis: "x",
                                vAxis: "t",
                                hMajorLines: true,
                                hMinorLines: true,
                                vMajorLines: false,
                                vMinorLines: false,
                                majorHLine: { 
                                                color: "gray", 
                                                width: 1, 
                                                style: "dash"
                                            },
                                minorHLine: { 
                                                color: "gray", 
                                                width: 1, 
                                                style: "dot" 
                                            } 
                            });
            
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
