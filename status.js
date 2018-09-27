var sts = null;

require([
    "dojo/dom", 
    "dojo/dom-attr",
    "dojo/dom-class",
    "dojo/dom-style",
    "dojo/html",
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
		status: null,		
		confirm: function(msg,ok,cmd){
			var dialog = new ConfirmDialog({
	        		title: "Conferma comando...",
	        		content: msg});
			dialog.set("buttonOk", ok);
			dialog.set("buttonCancel", "Annulla");
			dialog.on("execute", function(){
			putRequest("cgi-bin/command", cmd, 
				function(result){
					sts.update();
				},
				function(err){
					alert("Command error: " + err );
				});
	      });
			dialog.show();
		},
		flameout: new Button({
			label: "RESET FLAMEOUT!",
			disabled: true,
			class:"hidden",
			onClick: function(){sts.confirm("Reset pellet FLAMEOUT?", "reset flameout?","reset-flameout");}
      }, "pellet-flameout-reset-btn"),
      on: new Button({
          label: "Accendi impianto",
          disabled: true,
          onClick: function(){sts.confirm("Attivare l'impianto?","Attiva!","activate");}
      }, "on-btn"),
      off: new Button({
          label: "Spegni impianto",
          disabled: true,
          onClick: function(){sts.confirm("Disattivare l'impianto?","Disattiva!","deactivate");}
      }, "off-btn"),
      manual: new Button({
          label: "Manuale",
          disabled: true,
          onClick: function(){sts.confirm("Passare in MANUALE?","Manuale!","manual");}
      }, "manual-btn"),
      auto:new Button({
          label: "Automatico",
          disabled: true,
          onClick: function(){sts.confirm("Passare in AUTOMATICO?" + (sts.status.warnings.modeswitch != "" ? "<p>ATTENZIONE: " + sts.status.warnings.modeswitch+"</p>" : ""),"Automatico!","auto");}
      }, "auto-btn"),
      pelletOn:new Button({
          label: "Accendi",
          disabled: true,
          onClick: function(){sts.confirm("Accendo il PELLET?","Accendi!","pellet-on");}
      }, "pellet-on-btn"),
      pelletOff: new Button({
          label: "Spegni",
          disabled: true,
          onClick: function(){sts.confirm("Spengo il PELLET?","Spegni!","pellet-off");}
      }, "pellet-off-btn"),
      pelletMinOn: new Button({
          label: "minimo",
          disabled: true,
          onClick: function(){sts.confirm("Pellet al MINIMO?","Minimo!","pellet-minimum-on");}
      }, "pellet-minimum-on-btn"),
      pelletMinOff: new Button({
          label: "modula",
          disabled: true,
          onClick: function(){sts.confirm("Pellet in MODULAZIONE?","Modula!","pellet-minimum-off");}
      }, "pellet-minimum-off-btn"),
      gasOn: new Button({
          label: "Accendi",
          disabled: true,
          onClick: function(){sts.confirm("Accendo il GAS?","Accendi!","gas-on");}
      }, "gas-on-btn"),
      gasOff: new Button({
          label: "Spegni",
          disabled: true,
          onClick: function(){sts.confirm("Spengo il GAS?","Spegni!","gas-off");}
      }, "gas-off-btn"),
		tempMin: new Button({
			label: "XX.X°C",
			disabled: true,
			onClick: function(){
				eTempVal.set("value", sts.status.temp.min );
				eTemp.set("minOrMax", "min");
				eTemp.set("title", "Cambia temperatura minima")
				eTemp.show();		  
			}
		}, "min-temp"),          
		tempMax: new Button({
			label: "XX.X°C",
			disabled: true,
			onClick: function(){
				eTempVal.set("value", sts.status.temp.max );
				eTemp.set("minOrMax", "max");
				eTemp.set("title", "Cambia temperatura massima")
				eTemp.show();		  
			}
		}, "max-temp"),  
		saveTemp: function(m,v){	
			postRequest("cgi-bin/set_"+m+"_temp",{data:v},function(result){},function(err){alert("Command error: " + err );});
		},
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
						if ( sts[p] && sts[p].set ) sts[p].set("disabled", true); 
	            if ( sts.status.active == "on" ){
					sts.off.set("disabled", false );
					sts.tempMin.set("disabled", false);
					sts.tempMax.set("disabled", false);
					sts.tempMin.set("label", sts.status.temp.min + "°C" );
					sts.tempMax.set("label", sts.status.temp.max + "°C" );
				   
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
					html.set("temp-label",sts.status.temp.int + "C (" + sts.status.temp.ext + "C)" );
					html.set("humi-label", sts.status.temp.hum + "% (" + sts.status.temp.ext_hum + "%)" );
					prg.update(sts.status.program);
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
		},
	};
	
	for ( var p in sts ){				
		if ( sts[p] && sts[p].startup ) sts[p].startup();
	}	

});
