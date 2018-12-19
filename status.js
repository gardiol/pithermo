var sts = null;

require([
    "dojo/dom", 
    "dojo/dom-attr",
    "dojo/dom-class",
    "dojo/dom-style",
    "dojo/dom-construct",
    "dojo/html",
    "dojo/on",
    "dijit/ConfirmDialog",
    "dijit/form/Button", 
    "dijit/form/NumberSpinner",
    "dojo/domReady!"], 
function( dom, attr, dclass, style, dc, html, on,// Dojo
          ConfirmDialog, Button, NumberSpinner)// Dijit
{
    sts = {
        tempBts: {},
        timer: null,
        status: null,		
        confirm: function(msg,ok,cmd){
            var dialog = new ConfirmDialog({title: "Conferma comando...",content: msg});
            dialog.set("buttonOk", ok);
            dialog.set("buttonCancel", "Annulla");
            dialog.on("execute", function(){
            putRequest("cgi-bin/command", cmd, 
                function(result){sts.update();},
                function(err){alert("Command error: " + err );});
            });
            dialog.show();
        },
        updTempBts: function(v){
            var s=Math.floor(v)-2;
            for ( var r = 0; r < 5; r++ ){
                for ( var c = 0; c < 5; c++ ){                
                    var btn = sts.tempBts[r][c];
                    btn.set("label", (s+r)+"."+(c*2));
                }
            }    
        },
        setTemp:function(v,w,t){
            eTempVal.set("value", v );
            eTemp.set("minOrMax", w);
            eTemp.set("title", t);
            sts.updTempBts(v);
            eTemp.show();		  
        },        
        saveTemp: function(m,v){	
            postRequest("cgi-bin/set_"+m+"_temp",v,function(result){sts.update()},function(err){alert("Command error: " + err );});
        },
        hystEnable: function(){
            var e = hyst_enable.checked;
            hyst.set("disabled", !e );
            if ( e )
                dclass.remove(hyst_save.domNode, "hidden" );
            else
                dclass.add(hyst_save.domNode, "hidden" );
        },
        saveHyst: function(){
            postRequest("cgi-bin/set_hyst",hyst.get("value"),function(result){sts.update()},function(err){alert("Command error: " + err );});
            hyst.set("disabled", true );
            dclass.toggle(hyst_save.domNode, "hidden" );
            hyst_enable.set("checked", false );
        },
        
        disableAll:function(){
            dclass.add("pellet-flameout-led", "celated");
            pellet_on.set("disabled", true );
            pellet_off.set("disabled", true );
            pellet_minimum_on.set("disabled", true );
            pellet_minimum_off.set("disabled", true );
            gas_off.set("disabled", true );
            gas_on.set("disabled", true );
            status_master_on.set("disabled", true );
            status_master_off.set("disabled", true );
            status_manual.set("disabled", true );
            status_auto.set("disabled", true );
            min_temp.set("disabled", true);
            max_temp.set("disabled", true);
            min_temp_m.set("disabled", true);
			max_temp_m.set("disabled", true);
			min_temp_p.set("disabled", true);
			max_temp_p.set("disabled", true);
            min_temp.set("label", "XX.X°C" );
			max_temp.set("label", "XX.X°C" );
            hyst.set("value", "X.X");
        },

        update: function(){
            if ( sts.timer ){
                window.clearTimeout( sts.timer );
                sts.timer = null;
            }
            getRequest("cgi-bin/status",
                function(result){
                    if ( result ){
                        sts.status = result;
                        if ( sts.status.active == "on" ){
                            status_master_on.set("disabled", true );
                            status_master_off.set("disabled", false );

                            min_temp.set("disabled", false);
                            max_temp.set("disabled", false);
                            min_temp_m.set("disabled", false);
                            max_temp_m.set("disabled", false);
                            min_temp_p.set("disabled", false);
                            max_temp_p.set("disabled", false);
                            min_temp.set("label", sts.status.temp.min + "°C" );
                            max_temp.set("label", sts.status.temp.max + "°C" );
                            if ( ! hyst_enable.checked )
                                hyst.set("value", sts.status.temp.hys );

                            if ( sts.status.mode == "manual" ){
                                status_manual.set("disabled", true );
                                status_auto.set("disabled", false );
                                if ( sts.status.pellet.command == "on" ){
                                    pellet_off.set("disabled", false );
                                    pellet_on.set("disabled", true );
                                    if ( sts.status.pellet.minimum == "on" ){
                                        pellet_minimum_off.set("disabled", false );
                                        pellet_minimum_on.set("disabled", true );
                                    } else {
                                        pellet_minimum_off.set("disabled", true );
                                        pellet_minimum_on.set("disabled", false );
                                    }
                                } else {
                                    pellet_off.set("disabled", true );
                                    pellet_on.set("disabled", false );
                                    pellet_minimum_off.set("disabled", true );
                                    pellet_minimum_on.set("disabled", true );
                                }
                                if ( sts.status.gas.command == "on" ){
                                    gas_off.set("disabled", false );
                                    gas_on.set("disabled", true );
                                }
                                else{
                                    gas_off.set("disabled", true );
                                    gas_on.set("disabled", false );
                                }
                            } else if ( sts.status.mode == "auto" ) {
                                status_manual.set("disabled", false );
                                status_auto.set("disabled", true );
                                pellet_off.set("disabled", true );
                                pellet_on.set("disabled", true );
                                pellet_minimum_off.set("disabled", true );
                                pellet_minimum_on.set("disabled", true );
                                gas_off.set("disabled", true );
                                gas_on.set("disabled", true );
                            }
                            attr.set("mode-led", "src", sts.status.mode == "manual" ? "images/manual.png?1":"images/auto.png?1");                
                            attr.set("power-led", "src", sts.status.active != "on" ? "images/spento.png?1":"images/acceso.png?1");                
                            attr.set("pellet-feedback-led", "src", sts.status.pellet.status == "on" ? "images/max-temp.png?1":"images/min-temp.png?1");                
                            attr.set("pellet-minimum-status-led", "src", sts.status.pellet.minimum == "on" ? "images/pellet-minimo.png?1":"images/pellet-modulazione.png?1");
                            attr.set("pellet-status-led", "src", sts.status.pellet.command == "on" ? "images/pellet-on.png?1":"images/pellet-off.png?1");
                            attr.set("gas-status-led", "src", sts.status.gas.command == "on" ? "images/gas-on.png?1":"images/gas-off.png?1");    
                            if ( sts.status.pellet.flameout == "on" )
                                dclass.remove("pellet-flameout-led", "celated");
                            else
                                dclass.add("pellet-flameout-led", "celated");
                        } else { // not enabled
                            sts.disableAll();
                            status_master_on.set("disabled", false );
                        }
                        html.set("temp-label",sts.status.temp.int.toFixed(1) + "C (" + sts.status.temp.ext.toFixed(1) + "C)" );
                        html.set("humi-label", sts.status.temp.hum.toFixed(1) + "% (" + sts.status.temp.ext_hum.toFixed(1) + "%)" );
                        prg.update(sts.status.program);
                        html.set("update-time", utils.printDate(new Date()) + " (ok)" );
                    } // result is valid
                    sts.timer = window.setTimeout( function(){ sts.update(); }, 10*1000 );
                }, 
                function(err){
                    sts.status = null;
                    sts.disableAll();
                    html.set("temp-label", "--" );
                    html.set("humi-label", "--" );          
                    attr.set("pellet-feedback-led", "src", "images/min-temp.png?1");
                    attr.set("pellet-minimum-status-led", "src", "images/pellet-modulazione.png?1");
                    attr.set("pellet-status-led", "src", "images/pellet-off.png?1");
                    attr.set("gas-status-led", "src", "images/gas-off.png?1");
                    html.set("update-time", utils.printDate(new Date()) + " (ko)" );
                    sts.timer = window.setTimeout( function(){ sts.update(); }, 30*1000 );
                });
            }
        };
	
       
        for ( var r = 0; r < 5; r++ ){
            var x = dc.create("tr", null, dom.byId("eTempTable") );
            sts.tempBts[r] = {};
            for ( var c = 0; c < 5; c++ ){                
                sts.tempBts[r][c] = new Button({
                        label:'xx.x',
                        onClick: function(){
                            eTempVal.set("value", parseFloat(this.get("label")) );
                            sts.updTempBts(eTempVal.get("value"));
                        }
                    }, dc.create("td",{}, x) );
                sts.tempBts[r][c].startup();
            }
        }    
        
});
