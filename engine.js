// For program:
var prev_mode;
var minTemp;
var maxTemp;
var tempResetBtn;
var tempApplyBtn;
var prgResetBtn;
var prgApplyBtn;
var p_str = [];

var MessageStr = {
    0: "No event",
    1: "Sistema avviato",
    2: "Sistema fermato", 
    4: "Gas acceso",
    8: "Gas spento",
    16: "Pellet acceso",
    32: "Pellet spento",
    64: "Pellet al minimo",
    128: "Pellet in modulazione",
    256: "Pellet FLAMEOUT!",
    512: "Pellet flameout cancellato",
    1024: "Pellet mandata calda",
    2048: "Pellet mandata fredda",
    4096: "Sovra-temperatura rilevata!",
    8192: "Temperatura rientrata sotto il massimo",
    16384: "Temperatura sotto il minimo rilevata!",
    32768: "Temperatura rientrata sopra il minimo",
    65536: "Anti-ghiaccio attivato!",
    131072: "Anti-ghiaccio disattivato",
    262144: "Modo manuale impostato",
    524288: "Modo automatico impostato",
    1048576: "Temperatura minima modificata",
    2097152: "Temperatura massima modificata",
    4194304: "Programma modificato",
    8388608: "Sistema attivato",
    16777216: "Sistema disattivato",
};

require([
    "dojo/request",
    "dojo/dom", 
    "dojo/dom-attr",
    "dojo/dom-class",
    "dojo/dom-style",
    "dojo/dom-construct",
    "dojo/html",
    "dojo/query",
    "dojo/json",
    "dojo/on",
    "dojo/window",
    "dijit/registry",
    "dijit/ConfirmDialog",
    "dijit/layout/ContentPane",
    "dijit/layout/TabContainer",
    "dijit/form/Button", 
    "dijit/form/ToggleButton", 
    "dijit/form/NumberSpinner",
    "dijit/form/Select",
    "dijit/form/HorizontalSlider",
    "dojox/charting/Chart",
    "dojox/charting/axis2d/Default", 
    "dojox/charting/plot2d/Lines",
    "dojox/charting/themes/Chris",
    "dojox/charting/plot2d/Areas",
    "dojox/charting/plot2d/Markers",
    "dojox/charting/action2d/MouseIndicator",
    "dojo/domReady!"], 
function( request, dom, attr, dclass, style, domConstruct, html, query, json, on, win,     // Dojo
          registry, ConfirmDialog, ContentPane, TabContainer, Button, ToggleButton, NumberSpinner, Select, HorizontalSlider, // Dijit
          Chart, Default, Lines, Chris, Areas, Markers, MouseIndicator )               // Charing
{
    var hst = { 
        unit:new Select({
            onChange: changeHistory,
        },"history-unit"), 
        sel:new HorizontalSlider({
            value:1, 
            minimum: 1, 
            maximum:1,
            intermediateChanges: false, 
            discreteValues: 1,
            onChange: historySetData,
        },"history-sel"),
        grp: new Chart("history-graph",{ title: "Storico", titlePos: "bottom", titleGap: 25}),
        exp: new ToggleButton({ checked: false, onChange: function(v) {
            if ( v ){
                dclass.add(dom.byId("history-graph"), "history-big");
                this.set("label","riduci");
            } else {
                dclass.remove(dom.byId("history-graph"), "history-big");
                this.set("label","espandi");
            }
            hst.grp.resize();
        }}, "history-size"),
    };
    var hstTimer;
    var hstData = null;
    var extTempData = {};

    var stsTimer;
    var system_events = [];
    var sts = {
        flameout: new Button({
            label: "RESET FLAMEOUT!",
            disabled: true,
            class:"hidden",
            onClick: function(){confirmCmd("Reset pellet FLAMEOUT?", "reset flameout?","reset-flameout");}
        }, "pellet-flameout-reset-btn"),
        on: new Button({
            label: "Accendi impianto",
            disabled: true,
            onClick: function(){confirmCmd("Attivare l'impianto?","Attiva!","activate");}
        }, "on-btn"),
        off: new Button({
            label: "Spegni impianto",
            disabled: true,
            onClick: function(){confirmCmd("Disattivare l'impianto?","Disattiva!","deactivate");}
        }, "off-btn"),
        manual: new Button({
            label: "Manuale",
            disabled: true,
            onClick: function(){confirmCmd("Passare in MANUALE?","Manuale!","manual");}
        }, "manual-btn"),
        auto:new Button({
            label: "Automatico",
            disabled: true,
            onClick: function(){confirmCmd("Passare in AUTOMATICO?" + (system_status.warnings.modeswitch != "" ? "<p>ATTENZIONE: " + system_status.warnings.modeswitch+"</p>" : ""),"Automatico!","auto");}
        }, "auto-btn"),
        pelletOn:new Button({
            label: "Accendi",
            disabled: true,
            onClick: function(){confirmCmd("Accendo il PELLET?","Accendi!","pellet-on");}
        }, "pellet-on-btn"),
        pelletOff: new Button({
            label: "Spegni",
            disabled: true,
            onClick: function(){confirmCmd("Spengo il PELLET?","Spegni!","pellet-off");}
        }, "pellet-off-btn"),
        pelletMinOn: new Button({
            label: "minimo",
            disabled: true,
            onClick: function(){confirmCmd("Pellet al MINIMO?","Minimo!","pellet-minimum-on");}
        }, "pellet-minimum-on-btn"),
        pelletMinOff: new Button({
            label: "modula",
            disabled: true,
            onClick: function(){confirmCmd("Pellet in MODULAZIONE?","Modula!","pellet-minimum-off");}
        }, "pellet-minimum-off-btn"),
        gasOn: new Button({
            label: "Accendi",
            disabled: true,
            onClick: function(){confirmCmd("Accendo il GAS?","Accendi!","gas-on");}
        }, "gas-on-btn"),
        gasOff: new Button({
            label: "Spegni",
            disabled: true,
            onClick: function(){confirmCmd("Spengo il GAS?","Spegni!","gas-off");}
        }, "gas-off-btn"),          
        tempMin: new NumberSpinner({
            value: 0,
            disabled: true,
            smallDelta: 0.1,
            style: "width: 6em;",
            onChange: function(){ tempEdited = true; },
            constraints: { min:-100, max:100, places:1 }
        }, "min-temp"),
        tempMax: new NumberSpinner({
            value: 0,
            disabled: true,
            smallDelta: 0.1,
            style: "width: 6em;",
            onChange: function(){ tempEdited = true; },
            constraints: { min:-100, max:100, places:1 }
        }, "max-temp"),
        tempReset: new Button({
            label: "Ripristina",
            disabled: true,
            onClick: 	
                function(){
                    var dialog = new ConfirmDialog({
                        title: "ATTENZIONE!",
                        content: "Annullare le modifiche?"});
                    dialog.set("buttonOk", "Si, annulla");
                    dialog.set("buttonCancel", "No, continua");
                    dialog.on("execute", function() {
                        if ( system_status ) {
                            sts.tempMax.set("value", system_status.temp.max );				
                            sts.tempMin.set("value", system_status.temp.min );		
                            tempEdited = false;
                        }
                    });
                    dialog.show();
                }
        }, "temp-reset"),
        tempApply: new Button({
            label: "Applica",
            disabled: true,
            onClick: 
                function(){
                    var dialog = new ConfirmDialog({
                            title: "ATTENZIONE!",
                            content: "Salvare le modifiche?"});
                    dialog.set("buttonOk", "Salva");
                    dialog.set("buttonCancel", "Continua a modificare");
                    dialog.on("execute", function() {
                        request.post("cgi-bin/set_min_temp",{data:sts.tempMin.value}).then(
                        function(result){},
                        function(err){alert("Command error: " + err );});
                        request.post("cgi-bin/set_max_temp",{data:sts.tempMax.value}).then(
                        function(result){tempEdited = false;},
                        function(err){alert("Command error: " + err );});
                    });
                    dialog.show();
                }
        }, "temp-apply"),
    };   
    var system_status = null;
    var tempEdited = false;
    
    var prg = {
        reset:  new Button({
            label: "Ripristina",
            onClick: function(){
                var dialog = new ConfirmDialog({
                        title: "ATTENZIONE!",
                        content: "Annullare le modifiche?"});
                dialog.set("buttonOk", "Si, annulla");
                dialog.set("buttonCancel", "No, continua");
                dialog.on("execute", function() {
                    if ( system_status ) {
                        program_status = system_status.program;
                        programRefresh();
                    }
                });
                dialog.show();
            },
        }, "program-reset"),
        apply: new Button({
            label: "Applica",
            onClick: function(){
                var dialog = new ConfirmDialog({
                    title: "ATTENZIONE!",
                    content: "Salvare le modifiche?"});
                dialog.set("buttonOk", "Salva");
                dialog.set("buttonCancel", "Continua a modificare");
                dialog.on("execute", function() {
                    var ps = json.stringify(program_status);
                    request.post("cgi-bin/program",{data:ps}).then(
                        function(result){programEdited = false;},
                        function(err){alert("Command error: " + err );});
                });
                dialog.show();
            },
        }, "program-apply"),
    };
    var today_ref = [[],[],[]];
    var program_copy_d = [];
    var program_copy_h = [];
    var program_h_headers = [];
//    var program_f_headers = [];
    var program_d_headers = [
        domConstruct.create("th", {innerHTML:"LUN", colspan:2}),
        domConstruct.create("th", {innerHTML:"MAR", colspan:2}),
        domConstruct.create("th", {innerHTML:"MER", colspan:2}),
        domConstruct.create("th", {innerHTML:"GIO", colspan:2}),
        domConstruct.create("th", {innerHTML:"VEN", colspan:2}),
        domConstruct.create("th", {innerHTML:"SAB", colspan:2}),
        domConstruct.create("th", {innerHTML:"DOM", colspan:2})
    ];
    var program_cels = [];        
    var program_status = null;
    var selected_type;
    var copyInProgressD = null;
    var copyInProgressH = null;
    var programEdited = false;

    function confirmCmd(msg,ok,cmd){
		var dialog = new ConfirmDialog({
        		title: "Conferma comando...",
        		content: msg});
		dialog.set("buttonOk", ok);
		dialog.set("buttonCancel", "Annulla");
		dialog.on("execute", function(){
       		request.put("cgi-bin/command", {data:cmd}).then(
			function(result){
				updateStatus();
			},
			function(err){
				alert("Command error: " + err );
			});
        });
		dialog.show();
    }
         
    function changeHistory(){
        request.post("cgi-bin/set_history",{data:hst.unit.get("value")}).then(
            function(result){
                updateHistory();
            },
            function(err){
                alert("Command error: " + err );
            });
    }

	function selectType(t){
		selected_type = t;
        ["select-off", "select-gas", "select-pelletgas", "select-pellet", "select-pellet-minimum"].forEach( 
            function(o){dclass.remove(dom.byId(o), "program-selected");});
		if ( t == 'o' ){
			dclass.add(dom.byId("select-off"), "program-selected");
		}else if ( t == 'g' ){
			dclass.add(dom.byId("select-gas"), "program-selected");
		}else if ( t == 'x' ){
			dclass.add(dom.byId("select-pelletgas"), "program-selected");
		}else if ( t == 'p' ){
			dclass.add(dom.byId("select-pellet"), "program-selected");
		}else if ( t == 'm' ){
			dclass.add(dom.byId("select-pellet-minimum"), "program-selected");
		}
	}

	function programRefresh(){
        if ( !program_status || !system_status )
            return;
        programEdited = false;
        for ( var d = 0; (d < 7) && !programEdited; d++ ){
            for ( var h = 0; (h < 48) && !programEdited; h++ ){
                if ( program_status[d][h] != system_status.program[d][h] ){
                    programEdited = true;
                }
            }
        }
        if ( !programEdited ){
            dclass.add(dom.byId("program-change"), "celated");
        }else{
            dclass.remove(dom.byId("program-change"), "celated");
        }

        for ( var d = 0; d < 7; d++ ){
            
            for ( var h = 0; h < 24; h++ ){
                
                for ( var f = 0; f < 2; f++ ){
                    var c = program_status[d][h*2+f];
                    var src = "images/";
                    if ( c == 'p' ){
                        src += "pellet.png";
                    } else if ( c == 'g' ){
                        src += "gas.png";
                    } else if ( c == 'x' ){
                        src += "pellet-gas.png";
                    } else if ( c == 'm' ){
                        src += "pellet-min.png";
                    } else {
                        src += "off.png";
                    }
                    attr.set(program_cels[d][h][f]["_img"], "src", src );   
                    
                    dclass.remove(program_cels[d][h][f], "auto_now_now" )                                
                    dclass.remove(program_cels[d][h][f], "auto_now_c" );
                    if ( d == system_status.now.d ){
                        dclass.remove(today_ref[1+f][h], "auto_now_c" );
                        attr.set(today_ref[1+f][h]["_img"], "src", src );
                        if ( h == system_status.now.h ){
                            if ( f == system_status.now.f ){
                                dclass.add(program_cels[d][h][f], "auto_now_now" );                         
                                dclass.add(today_ref[1+f][h], "auto_now_c" );
                            } else {
                                dclass.add(program_cels[d][h][f], "auto_now_c" );     
                            }
                        }
                        else
                            dclass.add(program_cels[d][h][f], "auto_now_c" );                         
                    } else {
/*                        if ( (h == system_status.now.h) && (f == system_status.now.f) ) {
                            dclass.add(program_cels[d][h][f], "auto_now_c" );                         
                        }*/
                    }                        
                }
            }
        }
	}

    function buildEventStr(n){
            var t = new Date( system_events[n].t*1000).toLocaleString();
            var e_str = MessageStr[system_events[n].e] ? MessageStr[system_events[n].e] : MessageStr[0] + "(" + system_events[n].e + ")";                
            return t + " -- " + e_str;            
    }
	
	function updateStatus(){
        if ( stsTimer ){
            window.clearTimeout( stsTimer );
            stsTimer = null;
        }
        request("cgi-bin/status",{handleAs :"json"}).then(
            function(result){
                if ( result ){
                    system_status = result;
                    for ( var p in sts )
                        sts[p].set("disabled", true); 
                    sts.tempMin.set("disabled", false );
                    sts.tempMax.set("disabled", false );
                    if ( !tempEdited ){
                        if ( sts.tempMin.get("value") != system_status.temp.min )
                            sts.tempMin.set("value", system_status.temp.min);
                        if ( sts.tempMax.get("value") != system_status.temp.max )
                            sts.tempMax.set("value", system_status.temp.max);
                    }
                    sts.tempApply.set("disabled", false );
                    sts.tempReset.set("disabled", false );
                    if ( system_status.active == "on" )
                        sts.off.set("disabled", false );
                    else 
                        sts.on.set("disabled", false );
                    if ( system_status.mode == "manual" ){
                        sts.auto.set("disabled", false );
                        html.set("mode-label", "Impianto in MANUALE");
                        if ( system_status.pellet.command == "on" ){
                            sts.pelletOff.set("disabled", false );
                            if ( system_status.pellet.minimum == "on" )
                                sts.pelletMinOff.set("disabled", false );
                            else
                                sts.pelletMinOn.set("disabled", false );
                        }else{
                            sts.pelletOn.set("disabled", false );
                        }
                        if ( system_status.gas.command == "on" )
                            sts.gasOff.set("disabled", false );
                        else
                            sts.gasOn.set("disabled", false );
                    }else if ( system_status.mode == "auto" ) {
                        sts.manual.set("disabled", false );
                        html.set("mode-label", "Impianto in AUTOMATICO");
                    }else
                        html.set("mode-label", "Impianto SPENTO");
                    var p_h = Math.trunc(system_status.pellet.time/3600);
                    var p_m = Math.trunc((system_status.pellet.time/60)%60);
                    var mp_h = Math.trunc(system_status.pellet.mintime/3600);
                    var mp_m = Math.trunc((system_status.pellet.mintime/60)%60);
                    var Mp_h = Math.trunc((system_status.pellet.time-system_status.pellet.mintime)/3600);
                    var Mp_m = Math.trunc(((system_status.pellet.time-system_status.pellet.mintime)/60)%60);
                    html.set("pellet-time", p_h +"h" + p_m  + "m" );
                    html.set("pellet-mintime", mp_h +"h" + mp_m  + "m" );
                    html.set("pellet-modtime", Mp_h +"h" + Mp_m  + "m" );                    
                    html.set("gas-time", Math.trunc(system_status.gas.time/3600) +"h " + Math.trunc((system_status.gas.time/60)%60)  + "m");
                    html.set("temp-label",system_status.temp.int + "° (" + system_status.temp.ext + "°)" );
                    html.set("humi-label", system_status.temp.hum );
                    attr.set("pellet-feedback-led", "src", system_status.pellet.status == "on" ? "images/max-temp.png":"images/min-temp.png");                
                    attr.set("pellet-minimum-status-led", "src", system_status.pellet.minimum == "on" ? "images/pellet-minimo.png":"images/pellet-modulazione.png");
                    attr.set("pellet-status-led", "src", system_status.pellet.command == "on" ? "images/pellet-on.png":"images/pellet-off.png");
                    attr.set("gas-status-led", "src", system_status.gas.command == "on" ? "images/gas-on.png":"images/gas-off.png");                
                    style.set(sts.flameout.domNode, 'display', system_status.pellet.flameout == "on" ? 'inline' : 'none' );		
                    if ( !programEdited ){
                        program_status = [];
                        for ( var d = 0; d < system_status.program.length; d++ ){
                            program_status[d] = [];
                            for ( var h = 0; h < system_status.program[d].length; h++ )                                
                                program_status[d][h] = system_status.program[d][h];
                        }
                        programRefresh();
                    }
                    request("cgi-bin/events",{handleAs :"json"}).then(
                        function(events){
                            var s = system_events.length;
                            var e = events.length;
                            if ( (system_events == []) || 
                                (events.length < system_events.length) ){
                                domConstruct.empty("messages-queue");
                                s = 0;
                            }
                            system_events = events;                 
                            for ( var x = s; x < events.length; x++ ){
                                var str = buildEventStr(x);
                                domConstruct.place("<li>" + str + "</li>", "messages-queue","first");
                            }
                        },
                        function(err){
                            html.set("mode-label", "Impossibile leggere la lista degli eventi!");
                        });
                }
                stsTimer = window.setTimeout( function(){ updateStatus(); }, 2000 );
            }, 
            function(err){
                tempEdited = false;
                system_status = null;
                system_events = [];                
                for ( var p in sts )
                    sts[p].set("disabled", true);                
                html.set("gas-time", "Oggi: --h --m");
                html.set("pellet-time", "Oggi: --h --m");
                html.set("temp-label", "Temperatura: --(--)" );
                html.set("humi-label", "Umidità: --" );
                html.set("mode-label", "Connessione persa!");
                attr.set("pellet-feedback-led", "src", "images/min-temp.png");
                attr.set("pellet-minimum-status-led", "src", "images/pellet-modulazione.png");
                attr.set("pellet-status-led", "src", "images/pellet-off.png");
                attr.set("gas-status-led", "src", "images/gas-off.png");
                style.set(sts.flameout.domNode, 'display', 'none');		
                domConstruct.empty("messages-queue");
                domConstruct.place("<li>Connessione persa!</li>","messages-queue","first");
                stsTimer = window.setTimeout( function(){ updateStatus(); }, 5000 );
            });
	}

	function historySetData(){
        extTempData = {};
        var ext_zeros = 0;
        var t = [], h = [], x = [];
        var s = hst.sel.get("value");
        if ( hstData ){
            var te = [], ex = [], hu = [], ti = [];
            for ( var v = 0; (v < s) || (ti.length < 15); v++ ){
                if ( hstData[v] ){
                    te = hstData[v].temp.concat(te);
                    ex = hstData[v].ext_temp.concat(ex);
                    hu = hstData[v].humidity.concat(hu);
                    ti = hstData[v].time.concat(ti);
                } else {
                    break;
                }
            }
            var n_pts = ti.length;
            for ( p = 0; p < n_pts; p++ ){
                t.push( {x:ti[p], y:te[p] } );
                h.push( {x:ti[p], y:hu[p] } );
                x.push( {x:ti[p], y:ex[p] } );
                if ( ex[p] == 0 ) ext_zeros++;
                extTempData[ti[p]] = ex[p];
            }            
        }
        html.set("history-label", s);
        hst.grp.updateSeries("Temperatura", t.length == 0 ? [{x:0,y:0}] : t );
        if ( ext_zeros != x.length )
            hst.grp.updateSeries("Esterna", x.length == 0 ? [{x:0,y:0}] : x );
        else
            hst.grp.updateSeries("Esterna", [] );
        hst.grp.updateSeries("Umidita", h.length == 0 ? [{x:0,y:0}] : h );
        var ts = 60; // 1 min
        if ( t.length > 0 ){
            var d = t[ t.length-1 ].x - t[0].x;
            ts = Math.floor( ( d / Math.min(10, t.length) ) / (15*60) +1 ) * 15*60;
        }
        hst.grp.getAxis("x").opt.majorTickStep = ts;
        hst.grp.render();        
    }
    
	function updateHistory(){
            if ( hstTimer ){
                window.clearTimeout( hstTimer );
                hstTimer = null;
            }
       		request("cgi-bin/history" , {handleAs :"json"}).then(
			function(result){
                if ( result ) {
                    hstData = result.data;
                    if ( hst.unit.get("value") != result.mode )
                        hst.unit.set("value", result.mode);
                    if ( hst.sel.get("value") > result.len )
                        hst.sel.set("value", result.len);                        
                    hst.sel.set("maximum", result.len );
                    hst.sel.set("discreteValues", result.len );
                    historySetData();                                
                    html.set("history-label",  hst.sel.get("value"));
                    hstTimer = window.setTimeout( function(){ updateHistory(); }, 60 * 1000 );
                }
			},
			function(err){
                hstData = null;
                hst.grp.updateSeries("Temperatura", [] );
                hst.grp.updateSeries("Esterna", [] );
                hst.grp.updateSeries("Umidita", [] );
                hst.grp.render();
                html.set("history-label", "--");
				hstTimer = window.setTimeout( function(){ updateHistory(); }, 60 * 1000 );
			});
	}

	function buildHistory(){
        hst.unit.startup();
        hst.sel.startup();
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
                                if ( hst.unit.get("value") != "h" )
                                    return new Date(parseInt(value)*1000).toLocaleString();
                                else
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
        hst.grp.addSeries("Temperatura", [],{ plot: "tempPlot"});
        hst.grp.addSeries("Esterna", [],{ plot: "tempPlot", stroke: {color:"blue"} });
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
        hst.grp.addSeries("Umidita",[],{plot: "humiPlot"});
        new MouseIndicator(hst.grp, "humiPlot",{ 
                            series: "Umidita", start: true, mouseOver: true,
                            labelFunc: function(v){
                                return "H: "+v.y.toFixed(1)+" (" + (new Date(v.x*1000).toLocaleString())+")";
                            }
                            });
        new MouseIndicator(hst.grp, "tempPlot",{ 
                            series: "Temperatura",mouseOver: true,
                            labelFunc: function(v){
                                return "T: "+v.y.toFixed(1)+"/" + ((extTempData[v.x] != null) ? extTempData[v.x].toFixed(1) : "-");
                            }
                            });
    }
        
    function buildStatus(){
        new ToggleButton({ checked: false, onChange: function(v) {
            if ( v ){
                dclass.remove(dom.byId("status-controls"), "hidden");
                dclass.add(dom.byId("messages"), "messages-big");            
                this.set("label","riduci");
            } else {
                dclass.add(dom.byId("status-controls"), "hidden");
                dclass.remove(dom.byId("messages"), "messages-big");            
                this.set("label","espandi");
            }
        }}, "status-size");
        for ( var p in sts )
            sts[p].startup();
    }

    function buildProgram(){
        new ToggleButton({ checked: false, onChange: function(v) {
            if ( v ){
                dclass.remove(dom.byId("program-editor"), "hidden");
                dclass.add(dom.byId("today-table"), "hidden");
                this.set("label","riduci");
            } else {
                dclass.add(dom.byId("program-editor"), "hidden");
                dclass.remove(dom.byId("today-table"), "hidden");
                this.set("label","espandi");
            }
        }}, "program-size");
        on(dom.byId("select-off"), "click", function(){selectType('o');});
        on(dom.byId("select-gas"), "click", function(){selectType('g');});
        on(dom.byId("select-pelletgas"), "click", function(){selectType('x');});
        on(dom.byId("select-pellet"), "click", function(){selectType('p');});
        on(dom.byId("select-pellet-minimum"), "click", function(){selectType('m');});
        selectType('o');
        on(dom.byId("program-size"), "click", function(){
        });
        for ( var p in prg )
            prg[p].startup();
        
        
        for ( var h = 0; h < 24; h++ ){
            today_ref[0][h] = domConstruct.create("th", { class: "solid-down-sep", innerHTML: h < 10 ? "0"+h : h } );
            today_ref[1][h] = domConstruct.create("td", { class: "dashed-down-sep"} );
            today_ref[1][h]["_img"] = domConstruct.create("img", { src: "images/off.png" }, today_ref[1][h] );            
            today_ref[2].push( domConstruct.create("td", {} ) );
            today_ref[2][h]["_img"] = domConstruct.create("img", { src: "images/off.png" }, today_ref[2][h] );            
            
            program_h_headers[h] = domConstruct.create("th", { innerHTML: h < 10 ? "0"+h : h } );
            program_h_headers[h]["_h"] = h;
            on(program_h_headers[h], "click", function(evt){
                if ( program_status ){
                    var i = evt.currentTarget;
                    var h = i._h;
                    var dialog = new ConfirmDialog({title: "Imposta ora intera",
                                                    content: "Imposto l'ora uguale ora su tutta la settimana?"
                                                    });
                    dialog.set("buttonOk", "Si");
                    dialog.set("buttonCancel", "Annulla");
                    dialog.on("execute",function() {
                        for ( var d = 0; d < 7; d++ ){
                            for ( var f = 0; f < 2; f++ ){
                                program_status[d][h*2+f] = selected_type;
                            }
                        }
                        programRefresh();
                    });
                    dialog.show();
                }
            });
            program_copy_h[h] = domConstruct.create("td", {} );
            program_copy_h[h]["_h"] = h;
            program_copy_h[h]["_img"] = domConstruct.create("img", { class:"copy", src: "images/copy.png" }, program_copy_h[h] );            
            on(program_copy_h[h], "click", function(evt){
                if ( program_status ){
                    var i = evt.currentTarget;
                    var h = i._h;
                    if ( copyInProgressH === null ){
                        copyInProgressH = h;
                        for ( var x = 0; x < 24; x++ )
                            attr.set(program_copy_h[x]["_img"], "src", (x == copyInProgressH) ? "images/cancel_copy.png" : "images/paste.png");
                    } else {
                        for ( var x = 0; x < 24; x++ )
                            attr.set(program_copy_h[x]["_img"], "src", "images/copy.png" );
                        for ( var d = 0; d < 7; d++ ){
                            program_status[d][h*2] = program_status[d][copyInProgressH*2]; 
                            program_status[d][h*2+1] = program_status[d][copyInProgressH*2+1]; 
                        }
                        copyInProgressH = null;
                    }	
                    programRefresh();
                }
            });

            for ( var f = 0; f < 2; f++ ){
                for ( var d = 0; d < 7; d++ ){
                    if ( !program_cels[d] ) program_cels[d] = [];
                    if ( !program_cels[d][h] ) program_cels[d][h] = [];
                    program_cels[d][h][f] = ( domConstruct.create("td", { innerHTML: "" } ) );
                    program_cels[d][h][f]["_img"] = domConstruct.create("img", { src: "images/off.png" }, program_cels[d][h][f] );            
                    program_cels[d][h][f]["_d"] = d;
                    program_cels[d][h][f]["_h"] = h;
                    program_cels[d][h][f]["_f"] = f;
                    on(program_cels[d][h][f], "click", function(evt){
                        if ( program_status ){
                            var i = evt.currentTarget;
                            var x = i._h*2+i._f;
                            program_status[i._d][x] = program_status[i._d][x] == selected_type ? 'o' : selected_type;
                            programRefresh();
                        }
					});
                }
            }        
        }
        
        for ( var d = 0; d < 7; d++ ){
            program_copy_d[d] = domConstruct.create("td", { colspan:2 } );
            program_copy_d[d]["_d"] = d;
            program_copy_d[d]["_img"] = domConstruct.create("img", { class:"copy", src: "images/copy.png" }, program_copy_d[d] );            
            on(program_copy_d[d], "click", function(evt){
                if ( program_status ){
                    var i = evt.currentTarget;
                    var d = i._d;
                    if ( copyInProgressD === null ){
                        copyInProgressD = d;
                        for ( var x = 0; x < 7; x++ )
                            attr.set(program_copy_d[x]["_img"], "src", (x == copyInProgressD) ? "images/cancel_copy.png" : "images/paste.png");
                    } else {
                        for ( var x = 0; x < 7; x++ )
                            attr.set(program_copy_d[x]["_img"], "src", "images/copy.png" );
                        for ( var n = 0; n < program_status[d].length; n++ )
                            program_status[d][n] = program_status[copyInProgressD][n]; 	
                        copyInProgressD = null;
                    }	
                    programRefresh();
                }
            });
                                
            program_d_headers[d]["_d"] = d;
            on(program_d_headers[d], "click", function(evt){
                if ( program_status ){
                    var i = evt.currentTarget;
                    var d = i._d;
                    var dialog = new ConfirmDialog({title: "Imposta giornata",
                                                    content: "Imposto l'intero giorno?"});
                    dialog.set("buttonOk", "Si");
                    dialog.set("buttonCancel", "Annulla");
                    dialog.on("execute",function() {
                        for ( var h = 0; h < 24; h++ ){
                            for ( var f = 0; f < 2; f++ ){
                                program_status[d][h*2+f] = selected_type;
                            }
                        }
                        programRefresh();
                    });
                    dialog.show();
                }
            });
        }
        
        domConstruct.empty(dom.byId("program-table"));
        domConstruct.create("col", { span: 2}, domConstruct.create("colgroup", { class: "dayCol"} , dom.byId("program-table") ) );
        for ( var d = 0; d < 7; d++ )
            domConstruct.create("col", { span: 2}, domConstruct.create("colgroup", { class: "dayCol"} , dom.byId("program-table") ) );                 
        var row_node = domConstruct.create("tr", null, dom.byId("program-table") );
        domConstruct.create("td", { colspan:2, rowspan:3 }, row_node );
        for ( var d = 0; d < 7; d++ )
            domConstruct.place( program_copy_d[d], row_node );
        row_node = domConstruct.create("tr", null, dom.byId("program-table") );
        for ( var d = 0; d < 7; d++ )
            domConstruct.place( program_d_headers[d], row_node );
        row_node = domConstruct.create("tr", null, dom.byId("program-table") );
        for ( var d = 0; d < 7; d++ )
            for ( var f = 0; f < 2; f++ )
                domConstruct.create("th", { innerHTML: f == 0 ? "00" : "30" }, row_node );
        row_node = domConstruct.create("tr", null, dom.byId("program-table") );
        for ( var h = 0; h < 24; h++ ){
            domConstruct.place(program_copy_h[h], row_node);
            domConstruct.place(program_h_headers[h], row_node);
            for ( var d = 0; d < 7; d++ ){
                for ( var f = 0; f < 2; f++){
                    domConstruct.place(program_cels[d][h][f], row_node);
                }
            }
                row_node = domConstruct.create("tr", null, dom.byId("program-table") );
        }
        
        for ( var d = 0; d < 24; d++ )
            domConstruct.create("col", {class: "dayCol"}, dom.byId("today-table") );                 
        for ( var r = 0; r < 3; r++ ){
            var row_node = domConstruct.create("tr", null, dom.byId("today-table") );
            domConstruct.create("th", { class: r < 2 ? "solid-down-sep" : "", innerHTML: (r == 0 ? "" : (r == 1 ? "00" : "30"))}, row_node );            
            today_ref[r].forEach(function(c){
                domConstruct.place(c, row_node);
            });            
        };
    }
    
    buildHistory();
    buildStatus();
    buildProgram();                
	updateHistory();
	updateStatus();
    
//    on(window, "resize", function(){ console.log( win.getBox() ) } );
    
});
