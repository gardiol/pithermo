var system_status;

// History:
var historyGraph;

// For main are
var modeStack;
var autoPane;
var manualPane;
var programPane;
var autoBtn;
var manualBtn;
var programBtn;
// For status
var pelletOnBtn;
var pelletOffBtn;
var gasOnBtn;
var gasOffBtn;
var manualBtn;
var autoBtn;
var modeLabel;
// For auto:
var highlightCells;
// For program:
var prgResetBtn;
var prgApplyBtn;
var selectOff;
var selectGas;
var selectPellet;
var selectPelletGas;
var program_status;
var selected_type;
var program_linked;
var prev_mode;
var minTemp;
var maxTemp;
var tempResetBtn;
var tempApplyBtn;


require(["dijit/form/Button", 
	 "dojo/request",
	 "dojo/dom", 
	 "dojo/dom-attr",
	 "dojo/dom-class",
	 "dojo/dom-style",
	 "dojo/html",
	 "dojo/query",
	 "dojo/json",
	 "dijit/registry",
	 "dijit/ConfirmDialog",
	 "dijit/layout/ContentPane",
	 "dijit/layout/StackContainer",
	 "dijit/form/NumberSpinner",
	 "dojox/charting/Chart",
	 "dojox/charting/axis2d/Default", 
	 "dojox/charting/plot2d/Lines",
	 "dojo/on",
	 "dojo/domReady!"], 
function(Button, request, dom, attr, dclass, style, html, query, json, registry, ConfirmDialog, ContentPane, StackContainer, NumberSpinner, Chart, Default, Lines, on)
{
	function executeCommand(cmd) {
       		request.put("/cgi-bin/command", {data:cmd}).then(
			function(result){
				updateStatus();
			},
			function(err){
				alert("Command error: " + err );
			});
	}

	function pelletOn() {
		var dialog = new ConfirmDialog({
        		title: "ATTENZIONE!",
        		content: "Accendo il PELLET?"});
		dialog.set("buttonOk", "Accendi!");
		dialog.set("buttonCancel", "Annulla");
		dialog.on("execute", function(){executeCommand("pellet_on");});
		dialog.show();
	}
	function pelletOff() {
		var dialog = new ConfirmDialog({
        		title: "ATTENZIONE!",
        		content: "Spegno il PELLET?"});
		dialog.set("buttonOk", "Spegni!");
		dialog.set("buttonCancel", "Annulla");
		dialog.on("execute", function(){executeCommand("pellet_off");});
		dialog.show();
	}
	function gasOn() {
		var dialog = new ConfirmDialog({
        		title: "ATTENZIONE!",
        		content: "Accendo il GAS?"});
		dialog.set("buttonOk", "Accendi!");
		dialog.set("buttonCancel", "Annulla");
		dialog.on("execute", function(){executeCommand("gas_on");});
		dialog.show();
	}
	function gasOff() {
		var dialog = new ConfirmDialog({
        		title: "ATTENZIONE!",
        		content: "Spegno il GAS?"});
		dialog.set("buttonOk", "Spegni!");
		dialog.set("buttonCancel", "Annulla");
		dialog.on("execute", function(){executeCommand("gas_off");});
		dialog.show();
	}
	function setAutoMode() {
		var dialog = new ConfirmDialog({
        		title: "ATTENZIONE!",
        		content: "Pssare in automatico?"});
		dialog.set("buttonOk", "Passa in auto");
		dialog.set("buttonCancel", "Annulla");
		dialog.on("execute", function(){executeCommand("auto");});
		dialog.show();
	}
	function setManualMode() {
		var dialog = new ConfirmDialog({
        		title: "ATTENZIONE!",
        		content: "Pssare in manuale?"});
		dialog.set("buttonOk", "Passa in manuale");
		dialog.set("buttonCancel", "Annulla");
		dialog.on("execute", function(){executeCommand("manual");});
		dialog.show();
	}
	function autoRefresh(){
		if ( dom.byId( "auto-table" ) ) {
			var nd = system_status.now.d;
			var nh = system_status.now.h;
			var nf = system_status.now.f;
			if ( highlightCells )
				highlightCells.forEach( function(i,x){
						dclass.remove( i, "auto_now_h auto_now_c");
					});
			highlightCells = [];
			for ( var d = 0; d < 7; d++ ){
				if ( d == nd ){
					dclass.add( "auto-day-"+nd, "auto_now_h" );
					highlightCells.push( "auto-day-"+nd );
				}
				for ( var h = 0; h < 24; h++ ){
					if ( d == nd && h == nh ){
						dclass.add( "auto-header-"+(nh<10?"0":"")+nh, "auto_now_h" );
						highlightCells.push( "auto-header-"+(nh<10?"0":"")+nh );
					}
					for ( var f = 0; f < 2; f++ ){
						if ( d == nd && h == nh && f == nf ){
							dclass.add( "auto-header-"+(nh<10?"0":"")+nh+""+(nf==0?"00":"30"), "auto_now_h" );
							highlightCells.push( "auto-header-"+(nh<10?"0":"")+nh+""+(nf==0?"00":"30") );
						}
						var i = "auto-cell-"+d+"-"+(h < 10 ? "0"+h:h)+(f==0?"00":"30");
						var c = system_status.program[d][h*2+f];
						var s = c==''?"":'<img src="images/';
						if ( c == 'p' ) s+='pellet.png"/>';
						if ( c == 'g' ) s+='gas.png"/>';
						if ( c == 'x' ) s+='pellet-gas.png"/>';
						var n = dom.byId(i);
						html.set(n,s);
						if ( (d == nd) && (h!=nh || f!=nf)  ){
							dclass.add(n, "auto_now_c" );
							highlightCells.push( n );
						}else if ( (d != nd) && (h==nh && f == nf) ){
							dclass.add( n, "auto_now_c" );
							highlightCells.push( n );
						}else if ( d == nd && h == nh && f == nf ){
							dclass.add( "auto-cell-"+nd+"-"+(nh<10?"0":"")+nh+""+(nf==0?"00":"30"), "auto_now_h" );
							highlightCells.push( "auto-cell-"+nd+"-"+(nh<10?"0":"")+nh+""+(nf==0?"00":"30") );
						}
					}
				}
			}
		}
	}
	function selectType(t){
		selected_type = t;
		dclass.remove(selectOff, "program-selected");
		dclass.remove(selectGas, "program-selected");
		dclass.remove(selectPelletGas, "program-selected");
		dclass.remove(selectPellet, "program-selected");
		if ( t == '' ){
			dclass.add(selectOff, "program-selected");
		}else if ( t == 'g' ) {
			dclass.add(selectGas, "program-selected");
		}else if ( t == 'x' ) {
			dclass.add(selectPelletGas, "program-selected");
		}else if ( t == 'p' ) {
			dclass.add(selectPellet, "program-selected");
		}
	}
	function programReset() {
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
	}
	function programApply() {
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
						switchMode(system_status.mode);
					},
					function(err){
						alert("Command error: " + err );
					});
			});
		dialog.show();
	}
	function programRefresh(){
		if ( dom.byId( "program-table" ) ) {
			for ( var d = 0; d < 7; d++ ){
				for ( var h = 0; h < 24; h++ ){
					for ( var f = 0; f < 2; f++ ){
						var i = "program-cell-"+d+"-"+(h < 10 ? "0"+h:h)+(f==0?"00":"30");
						var c = program_status[d][h*2+f];
						var s = c==''?"":'<img src="images/';
						if ( c == 'p' ) s+='pellet.png"/>';
						if ( c == 'g' ) s+='gas.png"/>';
						if ( c == 'x' ) s+='pellet-gas.png"/>';
						var n = dom.byId(i);
						html.set(n,s);
					}
				}
			}
			if ( !program_linked )
				query("#program-table > tbody > tr > td")
					.on("click", 
					function(evt){ 
						var id = evt.currentTarget.id ;
						var d = id.substr(13,1);
						var h1 = id.substr(15,2);
						var h2 = id.substr(17,2);
						var h = (+h1)*2+(h2=="30"?1:0);
						program_status[d][h] = selected_type;
						programRefresh();
					});
			program_linked = true;
		}
	}

	function switchMode(mode){
		if ( mode == "auto" ){
			dclass.add( "modsel-auto", "modselected" );
			dclass.remove( "modsel-auto", "modunselected" );
			dclass.remove("modsel-manual", "modselected" );
			dclass.remove("modsel-program", "modselected" );
			dclass.add( "modsel-manual", "modunselected" );
			dclass.add( "modsel-program", "modunselected" );
			modeStack.selectChild( autoPane );
			autoRefresh();
		}else if ( mode == "manual" ){
			dclass.add( "modsel-manual", "modselected" );
			dclass.remove( "modsel-manual", "modunselected" );
			dclass.remove( "modsel-auto", "modselected" );
			dclass.remove( "modsel-program", "modselected" );
			dclass.add( "modsel-auto", "modunselected" );
			dclass.add( "modsel-program", "modunselected" );
			modeStack.selectChild( manualPane );
		}else if ( mode == "program" ){
			dclass.add( "modsel-program", "modselected" );
			dclass.remove( "modsel-program", "modunselected" );
			dclass.remove( "modsel-auto", "modselected" );
			dclass.remove( "modsel-manual", "modselected" );
			dclass.add( "modsel-auto", "modunselected" );
			dclass.add( "modsel-manual", "modunselected" );
			modeStack.selectChild( programPane );
		}
	}

	function updateStatus()
	{
       		request("cgi-bin/status" , {handleAs :"json"}).then(
			function(result)
			{
				system_status = result;
				if ( system_status.pellet.command == "on" ){
					attr.set("pellet-status-led", "src", "images/pellet-on.png");
					if ( system_status.mode == "manual" ){
						pelletOnBtn.set("disabled", true );
						pelletOffBtn.set("disabled", false );
					}
				}else{
					attr.set("pellet-status-led", "src", "images/pellet-off.png");
					if ( system_status.mode == "manual" ){
						pelletOnBtn.set("disabled", false );
						pelletOffBtn.set("disabled", true );
					}
				}
				if ( system_status.gas.command == "on" ){
					attr.set("gas-status-led", "src", "images/gas-on.png");
					if ( system_status.mode == "manual" ){
						gasOnBtn.set("disabled", true );
						gasOffBtn.set("disabled", false );
					}
				}else{
					attr.set("gas-status-led", "src", "images/gas-off.png");
					if ( system_status.mode == "manual" ){
						gasOnBtn.set("disabled", false );
						gasOffBtn.set("disabled", true );
					}
				}
				if ( system_status.mode != "auto" ){
					autoBtn.set("disabled", false );
					manualBtn.set("disabled", true );
					html.set(modeLabel, "Impianto in MANUALE");
				}else{
					autoBtn.set("disabled", true );
					manualBtn.set("disabled", false );
					gasOnBtn.set("disabled", true );
					gasOffBtn.set("disabled", true );
					pelletOnBtn.set("disabled", true );
					pelletOffBtn.set("disabled", true );
					html.set(modeLabel, "Impianto in AUTOMATICO");
				}
				window.setTimeout( function(){ updateStatus(); }, 5000 );
			}, 
			function(err)
			{
				alert("Impossibile caricare lo stato del sistema: "+err);
			})

	}

	function updateHistory()
	{
       		request("cgi-bin/history" , {handleAs :"json"}).then(
			function(result){
				historyGraph.updateSeries("Temp", result );
				historyGraph.render();
				window.setTimeout( function(){ updateHistory(); }, 60 * 1000 );
			},
			function(err){
				alert("Impossibile caricare la storia: "+err);
			});
	}







	modeStack = new StackContainer({}, 
		"mode-stack");
	autoPane = new ContentPane({
		href: "auto.html",
		onLoad: function() {
				autoRefresh();
			}
		}, "auto-pane" );
	autoPane.startup();
	manualPane = new ContentPane({
		href: "status.html",
		onLoad: function() {
				modeLabel = dom.byId( "mode-label" );
				manualBtn = new Button({
					label: "Manuale",
					disabled: true,
					onClick: setManualMode
				}, "manual-btn");
				autoBtn = new Button({
					label: "Automatico",
					disabled: true,
					onClick: setAutoMode
				}, "auto-btn");
				pelletOnBtn = new Button({
					label: "Accendi",
					disabled: true,
					onClick: pelletOn
				}, "pellet-on-btn");
				pelletOnBtn.startup();
				pelletOffBtn = new Button({
					label: "Spegni",
					disabled: true,
					onClick: pelletOff
				}, "pellet-off-btn");
				pelletOffBtn.startup();
				gasOnBtn = new Button({
					label: "Accendi",
					disabled: true,
					onClick: gasOn
				}, "gas-on-btn");
				gasOnBtn.startup();
				gasOffBtn = new Button({
					label: "Spegni",
					disabled: true,
					onClick: gasOff
				}, "gas-off-btn");
				gasOffBtn.startup();
			}
		}, "manual-pane" );
	manualPane.startup();
	programPane = new ContentPane({
		href: "program.html",
		onLoad: function() {
				prgResetBtn = new Button({
					label: "Ripristina",
					onClick: programReset
				}, "program-reset");
				prgResetBtn.startup();
				prgApplyBtn = new Button({
					label: "Applica",
					onClick: programApply
				}, "program-apply");
				prgApplyBtn.startup();
				program_status = system_status.program;
				programRefresh();
				selectOff = dom.byId("select-off");
				on( selectOff, "click", function(){selectType('');});
				selectGas = dom.byId("select-gas");
				on( selectGas, "click", function(){selectType('g');});
				selectPelletGas = dom.byId("select-pelletgas");
				on( selectPelletGas, "click", function(){selectType('x');});
				selectPellet = dom.byId("select-pellet");
				on( selectPellet, "click", function(){selectType('p');});
				selectType('');
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
					//onClick: programApply
				}, "temp-reset");
				tempApplyBtn = new Button({
					label: "Applica",
					//onClick: programApply
				}, "temp-apply");
			}
		}, "program_pane" );
	programPane.startup();
	modeStack.addChild( autoPane );
	modeStack.addChild( manualPane );
	modeStack.addChild( programPane );
	modeStack.selectChild( manualPane );
	modeStack.startup();

	manualBtn = new Button({
		label: "Stato",
		onClick: function(){switchMode("manual");}
		}, "modsel-manual-btn");
	manualBtn.startup();
	autoBtn = new Button({
		label: "Programma attivo",
		onClick: function(){switchMode("auto");}
		}, "modsel-auto-btn");
	autoBtn.startup();
	programBtn = new Button({
		label: "Cambia programma",
		onClick: function(){switchMode("program");}
		}, "modsel-program-btn");
	programBtn.startup();
	switchMode("manual");
	

	updateStatus();
	historyGraph = new Chart("history-graph");
	historyGraph.addPlot("default", {type: Lines});
	historyGraph.addAxis("x");
	historyGraph.addAxis("y", {vertical: true});
	historyGraph.addSeries("Temp", [], {stroke: {color: "red", width: 2}} );
	historyGraph.render();
	updateHistory();
});