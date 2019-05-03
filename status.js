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
        min_temp: null,
        max_temp: null,
        excess_temp: null,
        manual_off_edit: false,
        smart_temp: false,
	
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
            var s=Math.floor(v)-1;
            for ( var r = 0; r < 5; r++ ){
                for ( var c = 0; c < 5; c++ ){                
                    var btn = sts.tempBts[r][c];
                    btn.set("label", s.toFixed(1) );
                    s+=0.1;
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
        hystEnable: function(w){
            if ( w ){
                var e = hyst_max_enable.checked;
                hyst_max.set("disabled", !e );
                if ( e )
                    dclass.remove(hyst_max_save.domNode, "hidden" );
                else
                    dclass.add(hyst_max_save.domNode, "hidden" );
            }else{
                var e = hyst_min_enable.checked;
                hyst_min.set("disabled", !e );
                if ( e )
                    dclass.remove(hyst_min_save.domNode, "hidden" );
                else
                    dclass.add(hyst_min_save.domNode, "hidden" );
            }
        },
        saveHyst: function(w){
            if ( w ){
                putRequest("cgi-bin/command","set-hyst-max"+hyst_max.get("value"),function(result){sts.update()},function(err){alert("Command error: " + err );});
                hyst_max.set("disabled", true );
                dclass.toggle(hyst_max_save.domNode, "hidden" );
                hyst_max_enable.set("checked", false );
            }else{
                putRequest("cgi-bin/command","set-hyst-min"+hyst_min.get("value"),function(result){sts.update()},function(err){alert("Command error: " + err );});
                hyst_min.set("disabled", true );
                dclass.toggle(hyst_min_save.domNode, "hidden" );
                hyst_min_enable.set("checked", false );
            }
        },
        toggleSmart: function(){ 
            smart_temp_on.set("checked", sts.smart_temp);
            var dialog = new ConfirmDialog({title: "Smart Temp",
                content: !sts.smart_temp ? "Abilito la smart temp?" : "Disabilito la smart temp?"});
            dialog.set("buttonOk", "Si");
            dialog.set("buttonCancel", "Annulla");
            dialog.on("execute", function(){
                putRequest("cgi-bin/command", sts.smart_temp ? "smart-temp-off" : "smart-temp-on", 
                    function(result){sts.update();},
                    function(err){alert("Command error: " + err );});
            });
            dialog.show();
        },
        
        disableAll:function(){
            dclass.add("auto-pane-set", "hidden" );
            dclass.add("manual-pane-set", "hidden" );
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
            smart_temp_on.set("disabled", true);
            min_temp.set("disabled", true);
            max_temp.set("disabled", true);
            excess_temp.set("disabled", true);
            min_temp_m.set("disabled", true);
			max_temp_m.set("disabled", true);
			min_temp_p.set("disabled", true);
			max_temp_p.set("disabled", true);
            html.set("smart_temp", "off");
            min_temp.set("label", "XX.X°C" );
			max_temp.set("label", "XX.X°C" );
			excess_temp.set("label", "XX.X°C" );
            hyst_max.set("value", "X.X");
            hyst_min.set("value", "X.X");
        },
        
        saveOffTime: function(){
            var val = (manual_off_date.get("value") / 1000) + (manual_off_time.get("value" ) / 1000);
            putRequest("cgi-bin/command","set-mot"+val,function(result){sts.update()},function(err){alert("Command error: " + err );});
            manual_off_edit.set("checked", false );
            sts.manualOffEdit();
        },
        
        manualOffEnable: function(e,x){
            manual_off_edit.set("disabled", true);
            manual_off_date.set("disabled", true);
            manual_off_time.set("disabled", true);
            manual_off_save.set("disabled", true);
            manual_off_enable.set("disabled", true);
            manual_off_disable.set("disabled", true);
            if ( e ){
                manual_off_edit.set("disabled", false);
                manual_off_disable.set("disabled", false);
                if ( x ){
                    manual_off_save.set("disabled", false);
                    manual_off_edit.set("checked", true );
                    sts.manualOffEdit();
                }
            } else {
                manual_off_enable.set("disabled", false);
                if ( x ) {
                    manual_off_date.set("value", 0 );                
                    manual_off_time.set("value", 0 );                
                    sts.saveOffTime();
                }
            }
        },
        
        manualOffEdit: function(){
            if ( manual_off_edit.checked ){
                sts.manual_off_edit = true;
                manual_off_date.set("disabled", false);
                manual_off_time.set("disabled", false);
                manual_off_save.set("disabled", false);
            } else {
                manual_off_date.set("disabled", true);
                manual_off_time.set("disabled", true);
                manual_off_save.set("disabled", true);
                sts.manual_off_edit = false;
            }
        },
        
        update: function(){
            if ( sts.timer ){
                window.clearTimeout( sts.timer );
                sts.timer = null;
            }
            getRequest("cgi-bin/status",
                function(result){
                    var program = null;
                    var s = result.split(" ");
                    if ( s.length == 21 ){
                        sts.min_temp = parseFloat(s[10]);
                        sts.max_temp = parseFloat(s[9]);
                        sts.excess_temp = parseFloat(s[20]);
                        
                        if ( !sts.manual_off_edit ){
                            var n = parseInt(s[19])*1000;
                            var d = n == 0 ? new Date() :  new Date( n );
                            manual_off_date.set("value", d);
                            manual_off_time.set("value", d);                            
                            sts.manualOffEnable(n != 0, false );
                        }
                        
                        if ( s[1]=="1" ){//Active
                            attr.set("power-led", "src","images/acceso.png");                
                            status_master_on.set("disabled", true );
                            status_master_off.set("disabled", false );

                            min_temp.set("disabled", false);
                            max_temp.set("disabled", false);
                            min_temp_m.set("disabled", false);
                            max_temp_m.set("disabled", false);
                            min_temp_p.set("disabled", false);
                            max_temp_p.set("disabled", false);
                            excess_temp.set("disabled", false);
                            smart_temp_on.set("disabled", false);
                            min_temp.set("label", sts.min_temp.toFixed(1) + "°C" );
                            max_temp.set("label", sts.max_temp.toFixed(1) + "°C" );
                            excess_temp.set("label", sts.excess_temp.toFixed(1) + "°C" );
                            sts.smart_temp = s[17]=="1";//smart temp on
                            smart_temp_on.set("checked", sts.smart_temp);
                            if ( sts.smart_temp ){
                                html.set("smart_temp", parseFloat(s[18]).toFixed(1));
                            }else{
                                html.set("smart_temp", "off");
                            }
                            if ( ! hyst_max_enable.checked )                               
                                hyst_max.set("value", parseFloat(s[11]) ); //Hysteresis
                            if ( ! hyst_min_enable.checked )                               
                                hyst_min.set("value", parseFloat(s[12]) ); //Hysteresis
                            if ( s[3]=="1" ){//Manual mode
                                dclass.remove("manual-pane-set", "hidden" );
                                dclass.add("auto-pane-set", "hidden" );
                                attr.set("mode-led", "src", "images/manual.png");                
                                status_manual.set("disabled", true );
                                status_auto.set("disabled", false );
                                if ( s[4]=="1" ){//Pellet_on
                                    pellet_off.set("disabled", false );
                                    pellet_on.set("disabled", true );
                                    if ( s[5]=="1" ){//Pellet minimum
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
                                if ( s[8]=="1" ){ //Gas_on
                                    gas_off.set("disabled", false );
                                    gas_on.set("disabled", true );
                                }
                                else{
                                    gas_off.set("disabled", true );
                                    gas_on.set("disabled", false );
                                }
                            } else { // Mode non manual
                                dclass.remove("auto-pane-set", "hidden" );
                                dclass.add("manual-pane-set", "hidden" );
                                attr.set("mode-led", "src","images/auto.png");                
                                status_manual.set("disabled", false );
                                status_auto.set("disabled", true );
                                pellet_off.set("disabled", true );
                                pellet_on.set("disabled", true );
                                pellet_minimum_off.set("disabled", true );
                                pellet_minimum_on.set("disabled", true );
                                gas_off.set("disabled", true );
                                gas_on.set("disabled", true );
                            }
                            attr.set("pellet-status-led", "src",s[4]=="1" ? "images/pellet-on.png" : "images/pellet-off.png" );
                            attr.set("pellet-minimum-status-led", "src",s[5]=="1" ? "images/pellet-minimo.png" : "images/pellet-modulazione.png");
                            attr.set("gas-status-led", "src",s[8]=="1" ? "images/gas-on.png" : "images/gas-off.png");    
                            attr.set("pellet-feedback-led", "src", s[6] == "1" ? "images/max-temp.png":"images/min-temp.png");//Pellet HOT
                            if ( s[7]=="1" )// Flameout
                                dclass.remove("pellet-flameout-led", "celated");
                            else
                                dclass.add("pellet-flameout-led", "celated");
                        } else { // not enabled
                            attr.set("power-led", "src","images/spento.png");                
                            sts.disableAll();
                            status_master_on.set("disabled", false );
                        }
                        html.set("temp-label",parseFloat(s[13]).toFixed(1) + "C (" + parseFloat(s[15]).toFixed(1) + "C)" ); // t.int/ext
                        html.set("humi-label", parseFloat(s[14]).toFixed(1) + "% (" + parseFloat(s[16]).toFixed(1) + "%)" );// h.int/ext                        
                        
                        html.set("update-time", utils.printDate(new Date()) + " (ok)" );
                    } else {// result is valid
                        sts.disableAll();
                    }
                    sts.timer = window.setTimeout( function(){ sts.update(); }, 1000 );
                }, 
                function(err){
                    sts.disableAll();
                    html.set("temp-label", "--" );
                    html.set("humi-label", "--" );          
                    attr.set("pellet-feedback-led", "src", "images/min-temp.png");
                    attr.set("pellet-minimum-status-led", "src", "images/pellet-modulazione.png");
                    attr.set("pellet-status-led", "src", "images/pellet-off.png");
                    attr.set("gas-status-led", "src", "images/gas-off.png");
                    html.set("update-time", utils.printDate(new Date()) + " (ko)" );
                    sts.timer = window.setTimeout( function(){ sts.update(); }, 1000 );
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
