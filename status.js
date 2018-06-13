var sts = null;

require([
    "dojo/dom", 
    "dojo/dom-attr",
    "dojo/dom-class",
    "dojo/dom-style",
    "dojo/html",
    "dojo/query",
    "dojo/json",
    "dojo/on",
    "dijit/ConfirmDialog",
    "dijit/form/Button", 
    "dijit/form/NumberSpinner",
    "dojo/domReady!"], 
function( dom, attr, dclass, style, html, on,// Dojo
          ConfirmDialog, Button, NumberSpinner)// Dijit
{
	sts = {
		timer: null,
		inhibitTMin: false,
		inhibitTMax: false,
		tEdited: false,
		status: null,
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
          onClick: function(){confirmCmd("Passare in AUTOMATICO?" + (sts.status.warnings.modeswitch != "" ? "<p>ATTENZIONE: " + sts.status.warnings.modeswitch+"</p>" : ""),"Automatico!","auto");}
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
		tempReset: dom.byId("temp-reset"),
		tempApply: dom.byId("temp-apply"),
		tempMin: new NumberSpinner({
			value: 0,
         disabled: true,
         smallDelta: 0.1,
			intermediateChanges: true,
         style: "width: 6em;",
         onChange: function(){
				if ( !sts.inhibitTMin ){
					sts.tEdited = true;
					dclass.remove(tempReset, "celated");
					dclass.remove(tempApply, "celated");
				} else 
					sts.inhibitTMin = false;
			},
         constraints: { min:-100, max:100, places:1 }
		}, "min-temp"),
      tempMax: new NumberSpinner({
	      value: 0,
         disabled: true,
         smallDelta: 0.1,
			intermediateChanges: true,
         style: "width: 6em;",
         onChange: function(){ 
				if ( !sts.inhibitTMax ){
					sts.tEdited = true;
					dclass.remove(tempReset, "celated");
					dclass.remove(tempApply, "celated");
				} else
					sts.inhibitTMax = false;					
			},
         constraints: { min:-100, max:100, places:1 }
		}, "max-temp"),
		update: function(){
			if ( sts.timer ){
         	window.clearTimeout( sts.timer );
            sts.timer = null;
        	}
			getRequest("cgi-bin/status",
        	function(result){
				if ( result ){
					var next_update = 999999;
               sts.status = result;
               for ( var p in sts )
						if ( sts[p].set ) sts[p].set("disabled", true); 
	            if ( sts.status.active == "on" ){
	               sts.off.set("disabled", false );
		            if ( !sts.tEdited ){
							if ( sts.tempMin.get("value") != sts.status.temp.min ){
								sts.tempMin.set("value", sts.status.temp.min);
								sts.inhibitTMin = true;
							}
							if ( sts.tempMax.get("value") != sts.status.temp.max ){
								sts.tempMax.set("value", sts.status.temp.max);
								sts.inhibitTMax = true;
							}
		            }
						sts.tempMin.set("disabled", false );
						sts.tempMax.set("disabled", false );
						if ( sts.status.mode == "manual" ){
							sts.auto.set("disabled", false );
							if ( sts.status.pellet.command == "on" ){
								sts.pelletOff.set("disabled", false );
								if ( sts.status.pellet.minimum == "on" )
									sts.pelletMinOff.set("disabled", false );
								else
									sts.pelletMinOn.set("disabled", false );
							}else
								sts.pelletOn.set("disabled", false );
							if ( sts.status.gas.command == "on" )
								sts.gasOff.set("disabled", false );
							else
								sts.gasOn.set("disabled", false );
						} else if ( sts.status.mode == "auto" ) {
							sts.manual.set("disabled", false );						
						}
						var p_h = Math.trunc(sts.status.pellet.time/3600);
						var p_m = Math.trunc((sts.status.pellet.time/60)%60);
						var mp_h = Math.trunc(sts.status.pellet.mintime/3600);
						var mp_m = Math.trunc((sts.status.pellet.mintime/60)%60);
						var Mp_h = Math.trunc((sts.status.pellet.time-sts.status.pellet.mintime)/3600);
						var Mp_m = Math.trunc(((sts.status.pellet.time-sts.status.pellet.mintime)/60)%60);
						html.set("pellet-time", p_h +"h" + p_m  + "m" );
						html.set("pellet-mintime", mp_h +"h" + mp_m  + "m" );
						html.set("pellet-modtime", Mp_h +"h" + Mp_m  + "m" );                    
						html.set("gas-time", Math.trunc(sts.status.gas.time/3600) +"h " + Math.trunc((sts.status.gas.time/60)%60)  + "m");
						attr.set("mode-led", "src", sts.status.mode == "manual" ? "images/manual.png":"images/auto.png");                
						attr.set("power-led", "src", sts.status.active != "on" ? "images/spento.png":"images/acceso.png");                
						attr.set("pellet-feedback-led", "src", sts.status.pellet.status == "on" ? "images/max-temp.png":"images/min-temp.png");                
						attr.set("pellet-minimum-status-led", "src", sts.status.pellet.minimum == "on" ? "images/pellet-minimo.png":"images/pellet-modulazione.png");
						attr.set("pellet-status-led", "src", sts.status.pellet.command == "on" ? "images/pellet-on.png":"images/pellet-off.png");
						attr.set("gas-status-led", "src", sts.status.gas.command == "on" ? "images/gas-on.png":"images/gas-off.png");                
						style.set(sts.flameout.domNode, 'display', sts.status.pellet.flameout == "on" ? 'inline' : 'none' );		
						next_update = 2000;
					} else { // not enabled
	               sts.on.set("disabled", false );
						next_update = 15000;
					}
					html.set("temp-label",sts.status.temp.int + "° (" + sts.status.temp.ext + "°)" );
					html.set("humi-label", sts.status.temp.hum + "% (" + sts.status.temp.ext_hum + "%)" );
					prg.update();
					evt.update();
					sts.timer = window.setTimeout( function(){ sts.update(); }, next_update );
				} // result is valid
         }, 
			function(err){
				dclass.add(dom.byId("temp-reset"), "celated");
				dclass.add(dom.byId("temp-apply"), "celated");
				sts.status = null;
				html.set("temp-label", "--" );
				html.set("humi-label", "--" );
				for ( var p in sts )
					if ( sts[p].set ) sts[p].set("disabled", true);
				evt.disable();                
				html.set("gas-time", "--");
				html.set("pellet-time", "--");
				html.set("pellet-mintime", "--");
				html.set("pellet-modtime", "--");
				attr.set("pellet-feedback-led", "src", "images/min-temp.png");
				attr.set("pellet-minimum-status-led", "src", "images/pellet-modulazione.png");
				attr.set("pellet-status-led", "src", "images/pellet-off.png");
				attr.set("gas-status-led", "src", "images/gas-off.png");
				style.set(sts.flameout.domNode, 'display', 'none');		
				sts.timer = window.setTimeout( function(){ sts.update(); }, 5000 );
			});
		}
		build: function(){
			for ( var p in sts )
				if ( sts[p].startup ) sts[p].startup();

			on(sts.tempReset, "click", function(){
				var dialog = new ConfirmDialog({
					title: "ATTENZIONE!",
					content: "Annullare le modifiche?"});
				dialog.set("buttonOk", "Si, annulla");
				dialog.set("buttonCancel", "No, continua");
				dialog.on("execute", function() {
					if ( sts.status ) {
						if ( sts.tempMin.get("value") != system_status.temp.min ){
							sts.inhibitTMin = true;
						}
						if ( sts.tempMax.get("value") != system_status.temp.max ){
							sts.inhibitTMax = true;
						}
						sts.tempMax.set("value", system_status.temp.max );				
						sts.tempMin.set("value", system_status.temp.min );		
						sts.tEdited = false;
						dclass.add(dom.byId("temp-reset"), "celated");
						dclass.add(dom.byId("temp-apply"), "celated");
					}
				});
				dialog.show();
			});

			on(sts.tempApply, "click", function(){
				var dialog = new ConfirmDialog({
					title: "ATTENZIONE!",
					content: "Salvare le modifiche?"});
				dialog.set("buttonOk", "Salva");
				dialog.set("buttonCancel", "Continua a modificare");
				dialog.on("execute", function() {
					postRequest("cgi-bin/set_min_temp",sts.tempMin.value,
						function(result){
							sts.tEdited = false;
							dclass.add(dom.byId("temp-reset"), "celated");
							dclass.add(dom.byId("temp-apply"), "celated");
						},
						function(err){alert("Command error: " + err );});
					postRequest("cgi-bin/set_max_temp",sts.tempMax.value,
						function(result){
							sts.tEdited = false;
							dclass.add(dom.byId("temp-reset"), "celated");
							dclass.add(dom.byId("temp-apply"), "celated");
						},
						function(err){alert("Command error: " + err );});
				});
				dialog.show();
			});
    	}
	};
	
}
