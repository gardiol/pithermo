// Status
var system_status = null;
var firstStatusUpdate = true;
// History:
var hstGraph;
var hstUnit;
var hstData = null;
var hstSel;
var hstTimer;
// For main are
var modeTab;
var manualPane;
var programPane;
// For status
var pltOnBtn;
var pltMinBtn;
var pltOffBtn;
var pltModBtn;
var gasOnBtn;
var gasOffBtn;
var manualBtn;
var autoBtn;
// For program:
var copyInProgress = null;
var selectOff;
var selectGas;
var selectPellet;
var selectPelletMinimum;
var selectPelletGas;
var program_status;
var selected_type;
var prev_mode;
var minTemp;
var maxTemp;
var tempResetBtn;
var tempApplyBtn;
var prgResetBtn;
var prgApplyBtn;
var p_str = [];

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
    "dijit/registry",
    "dijit/ConfirmDialog",
    "dijit/layout/ContentPane",
    "dijit/layout/TabContainer",
    "dijit/form/Button", 
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
function( request, dom, attr, dclass, style, domConstruct, html, query, json, on,      // Dojo
          registry, ConfirmDialog, ContentPane, TabContainer, Button, NumberSpinner, Select, HorizontalSlider, // Dijit
          Chart, Default, Lines, Chris, Areas, Markers, MouseIndicator )               // Charing
{
    function confirmCmd(msg,ok,cmd){
		var dialog = new ConfirmDialog({
        		title: "Conferma comando...",
        		content: msg});
		dialog.set("buttonOk", ok);
		dialog.set("buttonCancel", "Annulla");
		dialog.on("execute", function(){
       		request.put("/cgi-bin/command", {data:cmd}).then(
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
        request.post("/cgi-bin/set_history",{data:hstUnit.get("value")}).then(
            function(result){
                updateHistory();
            },
            function(err){
                alert("Command error: " + err );
            });
    }

	function selectType(t){
		selected_type = t;
        [selectOff, selectGas, selectPelletGas, selectPellet, selectPelletMinimum].forEach( 
            function(o){
                dclass.remove(o, "program-selected");
            });
		if ( t == 'o' ){
			dclass.add(selectOff, "program-selected");
		}else if ( t == 'g' ){
			dclass.add(selectGas, "program-selected");
		}else if ( t == 'x' ){
			dclass.add(selectPelletGas, "program-selected");
		}else if ( t == 'p' ){
			dclass.add(selectPellet, "program-selected");
		}else if ( t == 'm' ){
			dclass.add(selectPelletMinimum, "program-selected");
		}
	}

	function programRefresh(){
        var eq = true;
		if ( dom.byId( "program-table" ) ){
			var nd = system_status.now.d, nh = system_status.now.h, nf = system_status.now.f;
			for ( var d = 0; d < 7; d++ ){
			   var n_g = 0, n_p = 0, n_P = 0;
				dclass.remove( p_str[d][24], "program_now_h" );
				for ( var h = 0; h < 24; h++ ){
                    dclass.remove( p_str[d][h][2], "program_now_h" );
					for ( var f = 0; f < 2; f++ ){
						var c = program_status[d][h*2+f];
                        var s = "";
                        if ( c != ' ' && c != 'o' ){
                            s = '<img src="images/';
                            if ( c == 'p' ){
                                s+='pellet.png"/>';
                                n_P++;
                            }else if( c == 'g' ){ 
                                s+='gas.png"/>';
                                n_g++;
                            }else if( c == 'x' ){
                                s+='pellet-gas.png"/>';
                                n_g++;
                                n_P++;
                            }else if( c == 'm' ) {
                                s+='pellet-min.png"/>';
                                n_p++;
                            }
                        }
                        html.set(p_str[d][h][f][0],s);
                        (d == nd || h == nh && f == nf) ? dclass.add(p_str[d][h][f][0], "auto_now_c" ) : dclass.remove(p_str[d][h][f][0], "auto_now_c" );
						dclass.remove( p_str[d][h][f][1], "program_now_h" );
                        if ( d == copyInProgress ){
                            dclass.add( p_str[d][h][f][0], "copy_source");
                        }else{                        
                            dclass.remove( p_str[d][h][f][0], "copy_source");
                        }
					}
				}
                 //Stima: al minimo 10h/1 sacco - modulazione: 5h/1 sacco
                var bags = (n_P/(2*5) + n_P/(2*8));
                var warn = (bags > 1.8) ? "<b>OCCHIO AL CONSUMO PELLET!</b>" : "";
                html.set("program-status-"+d, "Uso gas: "+(n_g/2)+"h -- Pellet: "+(n_P/2)+"h modulazione + "+(n_p/2)+" minimo -- Totale " + bags.toFixed(1) + " sacchi " + warn);
                if ( d == copyInProgress ){
                    dclass.add( "program-copy-"+d, "copy_source");
                    dclass.add( "program-status-"+d, "copy_source");
                }else{
                    dclass.remove( "program-copy-"+d, "copy_source");
                    dclass.remove( "program-status-"+d, "copy_source");
                }
                var l = program_status[d].length;
                while ( (l > 0) && eq ){
                    if ( program_status[d][l] != system_status.program[d][l] )
                        eq = false;
                    --l;
                }
			}
			dclass.add( p_str[nd][24], "program_now_h" );
            dclass.add( p_str[nd][nh][2], "program_now_h" );
			dclass.add( p_str[nd][nh][nf][1], "program_now_h" );
            if ( eq ){
               dclass.remove(dom.byId("program-change"), "program-changed");
               html.set("program-change", "");
            }else{
               dclass.add(dom.byId("program-change"), "program-changed");
               html.set("program-change", "Programma modificato!");
            }
		}                
	}

	function updateStatus(){
        request("cgi-bin/status",{handleAs :"json"}).then(
            function(result){
                system_status = result;
                [gasOnBtn,gasOffBtn,pltOnBtn,pltOffBtn,pltMinBtn,pltModBtn,manualBtn,autoBtn].forEach(
                    function(o){
                        o.set("disabled", true );
                    });
                if ( system_status.mode == "manual" ){
                    autoBtn.set("disabled", false );
                    html.set("mode-label", "Impianto in MANUALE");
                    if ( system_status.pellet.command == "on" )
                        pltOffBtn.set("disabled", false );
                    else
                        pltOnBtn.set("disabled", false );
                    if ( system_status.pellet.minimum == "on" )
                        pltModBtn.set("disabled", false );
                    else
                        pltMinBtn.set("disabled", false );
                    if ( system_status.gas.command == "on" )
                        gasOffBtn.set("disabled", false );
                    else
                        gasOnBtn.set("disabled", false );
                }else{
                    manualBtn.set("disabled", false );
                    html.set("mode-label", "Impianto in AUTOMATICO");
                }
                attr.set("pellet-feedback-led", "src", system_status.pellet.status == "on" ? "images/max-temp.png":"images/min-temp.png");                
                attr.set("pellet-minimum-status-led", "src", system_status.pellet.minimum == "on" ? "images/pellet-minimo.png":"images/pellet-modulazione.png");
                attr.set("pellet-status-led", "src", system_status.pellet.command == "on" ? "images/pellet-on.png":"images/pellet-off.png");
                attr.set("gas-status-led", "src", system_status.gas.command == "on" ? "images/gas-on.png":"images/gas-off.png");                
                domConstruct.empty("messages-queue");
                for ( var i = 0; i < system_status.warnings.messages.length; ++i )
                    domConstruct.place("<li>" + system_status.warnings.messages[i] + "</li>", "messages-queue","first");
                programPane.set("disabled", false );
                if (firstStatusUpdate) {
                    modeTab.selectChild(system_status.mode == "auto" ? programPane : manualPane);
                    firstStatusUpdate = false;
                }
                programRefresh();
                window.setTimeout( function(){ updateStatus(); }, 2000 );
            }, 
            function(err){
                system_status = null;
                modeTab.selectChild(manualPane);
                [autoBtn,manualBtn,gasOnBtn,gasOffBtn,pltOnBtn,pltOffBtn,
                 pltMinBtn,pltModBtn,programPane].forEach(
                    function(o){
                        o.set("disabled", true );
                    });
                html.set("mode-label", "Connessione persa!");
                attr.set("pellet-feedback-led", "src", "images/min-temp.png");
                attr.set("pellet-minimum-status-led", "src", "images/pellet-modulazione.png");
                attr.set("pellet-status-led", "src", "images/pellet-off.png");
                attr.set("gas-status-led", "src", "images/gas-off.png");
                domConstruct.empty("messages-queue");
                domConstruct.place("<li>Connessione persa!</li>","messages-queue","first");
                firstStatusUpdate = true;
                window.setTimeout( function(){ updateStatus(); }, 2000 );
            });
	}

	function historySetData(){
        var t = [], h = [];
        var s = hstSel.get("value");
        if ( hstData ){
            for ( var v = 0; (v < s) || (t.length < 15); v++ ){
		if ( hstData[v] ){
                	t = hstData[v].temp.concat(t);
                	h = hstData[v].humidity.concat(h);
		} else {
			break;
		}
            }
        }        
        html.set("history-label", s);
        hstGraph.updateSeries("Temperatura", t.length == 0 ? [{x:0,y:0}] : t );
        hstGraph.updateSeries("Umidita", h.length == 0 ? [{x:0,y:0}] : h );
        var ts = 60; // 1 min
        if ( t.length > 0 )
        {
            var d = t[ t.length-1 ].x - t[0].x;
            ts = Math.floor( ( d / Math.min(10, t.length) ) / (15*60) +1 ) * 15*60;
        }
        hstGraph.getAxis("x").opt.majorTickStep = ts;
        hstGraph.render();        
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
                    if ( hstUnit.get("value") != result.mode )
                        hstUnit.set("value", result.mode);
                    if ( hstSel.get("value") > result.len )
                        hstSel.set("value", result.len);                        
                    hstSel.set("maximum", result.len );
                    hstSel.set("discreteValues", result.len );
                    historySetData();                                
                    html.set("history-label",  hstSel.get("value"));
                    hstTimer = window.setTimeout( function(){ updateHistory(); }, 60 * 1000 );
                }
			},
			function(err){
                hstData = null;
                hstGraph.updateSeries("Temperatura", [] );
                hstGraph.updateSeries("Umidita", [] );
                hstGraph.render();
                html.set("history-label", "--");
				hstTimer = window.setTimeout( function(){ updateHistory(); }, 60 * 1000 );
			});
	}

    // Initialize all widgets
	manualPane = new ContentPane({
        href: "status.html",
        title: "Stato",
		onLoad: function() {
				manualBtn = new Button({
					label: "Manuale",
					disabled: true,
					onClick: function(){confirmCmd("Passare in MANUALE?","Manuale!","manual");}
				}, "manual-btn");
				autoBtn = new Button({
					label: "Automatico",
					disabled: true,
					onClick: function(){
                                confirmCmd("Passare in AUTOMATICO?" + 
                                        (system_status.warnings.modeswitch != "" ? "<p>ATTENZIONE: " + system_status.warnings.modeswitch+"</p>" : ""),
                                        "Automatico!","auto");}
				}, "auto-btn");
				pltOnBtn = new Button({
					label: "Accendi",
					disabled: true,
					onClick: function(){confirmCmd("Accendo il PELLET?","Accendi!","pellet-on");}
				}, "pellet-on-btn");
				pltOnBtn.startup();
				pltMinBtn = new Button({
					label: "minimo",
					onClick: function(){confirmCmd("Pellet al MINIMO?","Minimo!","pellet-minimum-on");}
				}, "pellet-minimum-on-btn");
				pltMinBtn.startup();
				pltOffBtn = new Button({
					label: "Spegni",
					disabled: true,
					onClick: function(){confirmCmd("Spengo il PELLET?","Spegni!","pellet-off");}
				}, "pellet-off-btn");
				pltOffBtn.startup();
				pltModBtn = new Button({
					label: "modula",
					onClick: function(){confirmCmd("Pellet in MODULAZIONE?","Modula!","pellet-minimum-off");}
				}, "pellet-minimum-off-btn");
				pltModBtn.startup();
				gasOnBtn = new Button({
					label: "Accendi",
					disabled: true,
					onClick: function(){confirmCmd("Accendo il GAS?","Accendi!","gas-on");}
				}, "gas-on-btn");
				gasOnBtn.startup();
				gasOffBtn = new Button({
					label: "Spegni",
					disabled: true,
					onClick: function(){confirmCmd("Spengo il GAS?","Spegni!","gas-off");}
				}, "gas-off-btn");
				gasOffBtn.startup();
			}}, "manual-pane" );    
	manualPane.startup();
    
	programPane = new ContentPane({
		href: "program.html",
        title: "Programma",
		onLoad: function() {
            	p_str = [];
                for ( var d = 0; d < 7; d++ ){
                    p_str[d] = [];
                    for ( var h = 0; h < 24; h++ ){
                        p_str[d][h] = [];
                        for ( var f = 0; f < 2; f++ ){
                            p_str[d][h][f] = [
                                "program-cell-"+d+"-"+(h < 10 ? "0"+h:h)+(f==0?"00":"30"),
                                "program-header-"+(h<10?"0":"")+h+""+(f==0?"00":"30")
                            ];
                        }
                        p_str[d][h][2] = "program-header-"+(h<10?"0":"")+h;
                    }
                    p_str[d][24] = "program-day-"+d;
                }
                query("#program-table > tbody > tr > td").on("click", 
					function(evt){ 
						var id = evt.currentTarget.id ;
						var d = id.substr(13,1);
						var h = (+id.substr(15,2))*2+(id.substr(17,2)=="30"?1:0);
						if ( (d>=0 && d < 7) && (h>=0 && h < 48) ){
                                                    program_status[d][h] = program_status[d][h] == selected_type ? 'o' : selected_type;
						    programRefresh();
                                                }
					});
                ["program-header-00","program-header-01","program-header-02","program-header-03","program-header-04","program-header-05",
                 "program-header-06","program-header-07","program-header-08","program-header-09","program-header-10","program-header-11",
                 "program-header-12","program-header-13","program-header-14","program-header-15","program-header-16","program-header-17",
                 "program-header-18","program-header-19","program-header-20","program-header-21","program-header-22","program-header-23"]
                .forEach( 
                    function(i){
                        on(dom.byId(i), "click", function(evt){
                            var h = evt.currentTarget.id.substr(15,2);
                            var dialog = new ConfirmDialog({
                                                            title: "Imposta ora intera",
                                                            content: "Imposto l'intera ora su tutta la settimana?"
                                                            });
                            dialog.set("buttonOk", "Si");
                            dialog.set("buttonCancel", "Annulla");
                            dialog.on("execute",
                                function() {
                                    for ( var d = 0; d < 7; d++ ){
                                        for ( var f = 0; f < 2; f++ ){
                                            program_status[d][h*2+f] = selected_type;
                                        }
                                    }
                                    programRefresh();
                                });
                            dialog.show();
                        });
                    });
                ["program-header-0000","program-header-0100","program-header-0200","program-header-0300","program-header-0400","program-header-0500",
                 "program-header-0600","program-header-0700","program-header-0800","program-header-0900","program-header-1000","program-header-1100",
                 "program-header-1200","program-header-1300","program-header-1400","program-header-1500","program-header-1600","program-header-1700",
                 "program-header-1800","program-header-1900","program-header-2000","program-header-2100","program-header-2200","program-header-2300",
                 "program-header-0030","program-header-0130","program-header-0230","program-header-0330","program-header-0430","program-header-0530",
                 "program-header-0630","program-header-0730","program-header-0830","program-header-0930","program-header-1030","program-header-1130",
                 "program-header-1230","program-header-1330","program-header-1430","program-header-1530","program-header-1630","program-header-1730",
                 "program-header-1830","program-header-1930","program-header-2030","program-header-2130","program-header-2230","program-header-2330"]
                .forEach( 
                    function(i){
                        on(dom.byId(i), "click", function(evt){
                            var id = evt.currentTarget.id;
                            var h = id.substr(15,2);
                            var f = id.substr(17,2) == "00" ? 0 : 1;
                            var dialog = new ConfirmDialog({
                        						title: "Imposta mezz'ora",
                        						content: "Imposto la mezz'ora su tutta la settimana?"});
                            dialog.set("buttonOk", "Si");
                            dialog.set("buttonCancel", "Annulla");
                            dialog.on("execute",
                                    function() {
								for ( var d = 0; d < 7; d++ ){
									program_status[d][h*2+f] = selected_type;
								}
								programRefresh();
							});
                            dialog.show();
                        });
                    });
                ["program-day-0","program-day-1","program-day-2","program-day-3","program-day-4","program-day-5","program-day-6"].forEach( 
                    function(i){
                        on(dom.byId(i), "click", function(evt){
                            var d = evt.currentTarget.id.substr(12,1);
                            var dialog = new ConfirmDialog({
                        						title: "Imposta giornata",
                        						content: "Imposto l'intero giorno?"});
                            dialog.set("buttonOk", "Si");
                            dialog.set("buttonCancel", "Annulla");
                            dialog.on("execute",
                                    function() {
                            for ( var h = 0; h < 24; h++ ){
                                for ( var f = 0; f < 2; f++ ){
                                    program_status[d][h*2+f] = selected_type;
                                }
                            }
                            programRefresh();
                            });
                            dialog.show();
                        });
                    });


			["copy0","copy1","copy2","copy3","copy4","copy5","copy6"].forEach(
				function(i){
					query("#"+i).on("click", 
						function(evt){
							if ( !copyInProgress ){
								copyInProgress = i.substr(4,1);
								for ( var x = 0; x < 7; x++ )
									attr.set("copy"+x, "src", (x == copyInProgress) ? "images/cancel_copy.png" : "images/paste.png");
							} else {
								var dest =  i.substr(4,1);
								program_status[dest] = program_status[copyInProgress]; 	
								for ( var x = 0; x < 7; x++ )
									attr.set("copy"+x, "src", "images/copy.png");
								copyInProgress = null;
							}	
							programRefresh();
						});
				});


				prgResetBtn = new Button({
					label: "Ripristina",
					onClick: 
                        function(){
                            var dialog = new ConfirmDialog({
                                    title: "ATTENZIONE!",
                                    content: "Annullare le modifiche?"});
                            dialog.set("buttonOk", "Si, annulla");
                            dialog.set("buttonCancel", "No, continua");
                            dialog.on("execute", function() {
                                            program_status = system_status.program;
                                            programRefresh();
                                        });
                            dialog.show();
                        },
				}, "program-reset");
				prgResetBtn.startup();
				prgApplyBtn = new Button({
					label: "Applica",
					onClick: 
                        function(){
                            var dialog = new ConfirmDialog({
                                title: "ATTENZIONE!",
                                content: "Salvare le modifiche?"});
                                dialog.set("buttonOk", "Salva");
                                dialog.set("buttonCancel", "Continua a modificare");
                                dialog.on("execute", 
                                    function() {
                                        var ps = json.stringify(program_status);
                                        request.post("/cgi-bin/program",{data:ps}).then(
                                            function(result){
                                            },
                                            function(err){
                                                alert("Command error: " + err );
                                            });
                                    });
                            dialog.show();
                        },
				}, "program-apply");
				prgApplyBtn.startup();
				program_status = system_status ? system_status.program : null;
				selectOff = dom.byId("select-off");
				on( selectOff, "click", function(){selectType('o');});
				selectGas = dom.byId("select-gas");
				on( selectGas, "click", function(){selectType('g');});
				selectPelletGas = dom.byId("select-pelletgas");
				on( selectPelletGas, "click", function(){selectType('x');});
				selectPellet = dom.byId("select-pellet");
				on( selectPellet, "click", function(){selectType('p');});
				selectPelletMinimum = dom.byId("select-pellet-minimum");
				on( selectPelletMinimum, "click", function(){selectType('m');});
				selectType('o');
				minTemp = new NumberSpinner({
					value: system_status.temp.min,
					smallDelta: 0.1,
					style: "width: 6em;",
        				constraints: { min:0, max:25, places:1 }
    				}, "min-temp");
				minTemp.startup();
				maxTemp = new NumberSpinner({
					value: system_status.temp.max,
					smallDelta: 0.1,
					style: "width: 6em;",
        				constraints: { min:0, max:25, places:1 }
    				}, "max-temp");
				maxTemp.startup();
				tempResetBtn = new Button({
					label: "Ripristina",
					onClick: 	
                        function(){
                            var dialog = new ConfirmDialog({
                                title: "ATTENZIONE!",
                                content: "Annullare le modifiche?"});
                            dialog.set("buttonOk", "Si, annulla");
                            dialog.set("buttonCancel", "No, continua");
                            dialog.on("execute", function() {
                                            maxTemp.set("value", system_status.temp.max );				
                                            minTemp.set("value", system_status.temp.min );				
                                        });
                            dialog.show();
                        }
				}, "temp-reset");
				tempApplyBtn = new Button({
					label: "Applica",
					onClick: 
                        function(){
                            var dialog = new ConfirmDialog({
                                    title: "ATTENZIONE!",
                                    content: "Salvare le modifiche?"});
                            dialog.set("buttonOk", "Salva");
                            dialog.set("buttonCancel", "Continua a modificare");
                            dialog.on("execute", 
                                function() {
                                        request.post("/cgi-bin/set_min_temp",{data:minTemp.value}).then(
                                        function(result){
                                        },
                                        function(err){
                                            alert("Command error: " + err );
                                        });
                                        request.post("/cgi-bin/set_max_temp",{data:maxTemp.value}).then(
                                        function(result){
                                        },
                                        function(err){
                                            alert("Command error: " + err );
                                        });
                                });
                            dialog.show();
                        }
				}, "temp-apply");
                programRefresh();
			}}, "program_pane" );
	programPane.startup();
    
	modeTab = new TabContainer({style: "height: 100%; width: 100%;"}, "mode-stack");
	modeTab.addChild( manualPane );
	modeTab.addChild( programPane );
	modeTab.startup();        
    modeTab.selectChild( manualPane );
    
	updateStatus();
    
    hstUnit = new Select({},"history-unit");
    hstUnit.on("change", changeHistory);
    hstUnit.startup();
    
    hstSel = new HorizontalSlider({
        value:1, minimum: 1, maximum:1,
        intermediateChanges: false, discreteValues: 1,
        onChange: historySetData,
        },"history-sel");
    hstSel.startup();
    
    hstGraph = new Chart("history-graph",{ title: "Storico", titlePos: "bottom", titleGap: 25});
    hstGraph.setTheme(Chris);
    hstGraph.addPlot("tempPlot",{
                        type: Lines,lines: true, areas: false, markers: false,
                        tension: "X",
                        stroke: {color: "red",  width: 1}
                    });
    hstGraph.addAxis("x", 	{
                        plot:"tempPlot", 
                        majorTicks: true, majorLabels: true,
                        minorTicks: false,minorLabels: false,
                        microTicks: false,
                        labelFunc:function(text,value,prec){
                            if ( hstUnit.get("value") != "h" )
                                return new Date(parseInt(value)*1000).toLocaleString();
                            else
                                return new Date(parseInt(value)*1000).toLocaleTimeString();
                        }
                    });
    hstGraph.addAxis("y", 	{
                        plot:"tempPlot", 
                        vertical: true, 
                        dropLabels: false,
                        majorTickStep: 10, majorTicks: true, majorLabels: true,
                        minorTickStep: 1, minorTicks: true, minorLabels: true,
                        microTickStep: 0.1, microTicks: true,
                        fixLower: "major",  fixUpper: "major"
                    });
    hstGraph.addSeries("Temperatura", [],{ plot: "tempPlot"});
    hstGraph.addPlot("humiPlot",{
                        type: Lines,lines: true, areas: false, markers: false,
                        tension: "X",
                        hAxis: "x",vAxis: "h",
                        stroke: {color: "yellow", width: 1	}
                    });
    hstGraph.addAxis("h", 	{
                        plot:"humiPlot", 
                        vertical: true, leftBottom: false,
                        majorTickStep: 10, 
                            minorTickStep: 1,
                        fixLower: "major", fixUpper: "major"
                    });
    hstGraph.addSeries("Umidita",[],{plot: "humiPlot"});
    new MouseIndicator(hstGraph, "humiPlot",{ 
                        series: "Umidita", start: true, mouseOver: true,
                        labelFunc: function(v){
                            return "H: "+v.y;
                        }
                        });
    new MouseIndicator(hstGraph, "tempPlot",{ 
                        series: "Temperatura",mouseOver: true,
                        labelFunc: function(v){
                            return "T: "+v.y+" (" + (new Date(v.x*1000).toLocaleString())+")";
                        }
                        });                    
	updateHistory();
    
});
