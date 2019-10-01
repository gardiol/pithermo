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
          ConfirmDialog )// Dijit
{
    sts = {
        timer: null,
	
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
        setTemp:function(w){
            var v="xx.x",t="err";         
            if ( w == 0 ){
                v = min_temp.get('label');
                t = "Cambia temperatura minima";
            }else if(w==1){
                v = max_temp.get('label');
                t = "Cambia temperatura massima";
            }else if(w==2){
                v = min_hyst.get('label');
                t = "Cambia isteresi minima";
            }else if(w==3){
                v = max_hyst.get('label');
                t = "Cambia isteresi massima";
            }else if(w==4){
                v = excess_temp.get('label');
                t = "Cambia soglia di eccesso";
            }            
            eTemp.set("from", w);   
            eTemp.set("title", t );
            eTempVal.set("value", parseFloat(v) );
            eTemp.show();		  
        },        
        saveTemp: function(){	
            var m = eTemp.attr('from');
            var v = eTempVal.get('value');
            var c = "";
            if ( m == 0 ){
                c = "min-temp"+v;
            }else if ( m == 1 ){
                c = "max-temp"+v;
            }else if ( m == 2 ){
                c = "set-hyst-min"+v;
            }else if ( m == 3 ){
                c = "set-hyst-max"+v;
            }else if ( m == 4 ){
                c = "exc-temp"+v;
            }
            putRequest("cgi-bin/command",c,function(result){sts.update()},function(err){alert("Command error: " + err );});
        },
        toggleSmart: function(){ 
            var smart_temp = !smart_temp_on.checked; // due to state change on click
            var dialog = new ConfirmDialog({title: "Smart Temp",
                content: !smart_temp ? "Abilito la temperatura smart?" : "Disabilito la temperatura smart?"});
            dialog.set("buttonOk", "Si");
            dialog.set("buttonCancel", "Annulla");
            dialog.on("execute", function(){
                putRequest("cgi-bin/command", smart_temp ? "smart-temp-off" : "smart-temp-on", 
                    function(result){sts.update();},
                    function(err){alert("Command error: " + err );});
            });
            dialog.show();
        },
        
        saveOffTime: function(){
            var v = eOffTime.attr("off-time") - (new Date() / 1000);
            putRequest("cgi-bin/command","set-mot"+v,function(result){sts.update()},function(err){alert("Command error: " + err );});
        },
        disableOffTime: function(){
            putRequest("cgi-bin/command","set-mot0",function(result){sts.update()},function(err){alert("Command error: " + err );});
        },
        setOffTime: function(){
            var v = Math.floor( new Date() / 1000 );
            eOffTime.set("off-time", v);          
            sts.updateOffTime(1800);
            eOffTime.show();
        },
        updateOffTime: function(v){
            eOffTime.set("off-time", eOffTime.attr("off-time")+v );          
            eOffTimeEdit.set("value", utils.printDate( eOffTime.attr("off-time")*1000 ) );
        },
        
        disableAll:function(){
            manual_off_time.set("disabled", true );
            manual_off_time.set("label", "--:--" );
            dclass.add("excess-temp-detected", "hidden");
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
            min_temp.set("label", "XX.X°C" );
            max_temp.set("label", "XX.X°C" );
            excess_temp.set("label", "XX.X°C" );
            hyst_max.set("value", "X.X");
            hyst_min.set("value", "X.X");
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
                    if ( s.length == 22 ){
                        
                        if ( s[21] =="1")
                            dclass.remove("excess-temp-detected", "hidden");
                        else
                            dclass.add("excess-temp-detected", "hidden");
                        
                        if ( s[1]=="1" ){//Active
                            attr.set("power-led", "src","images/acceso.png");                
                            status_master_on.set("disabled", true );
                            status_master_off.set("disabled", false );

                            min_temp.set("disabled", false);
                            max_temp.set("disabled", false);
                            excess_temp.set("disabled", false);
                            smart_temp_on.set("disabled", false);
                            manual_off_time.set("disabled", false);

                            min_temp.set("label", parseFloat(s[10]).toFixed(1) + "°C" );
                            max_temp.set("label", parseFloat(s[9]).toFixed(1) + "°C" );
                            min_hyst.set("label", parseFloat(s[12]).toFixed(1) + "°C" );
                            max_hyst.set("label", parseFloat(s[11]).toFixed(1) + "°C" );
                            excess_temp.set("label", parseFloat(s[20]).toFixed(1) + "°C" );
                            if ( s[17]=="1" ){ //smart temp on
                                smart_temp_on.set("checked", true);
                                smart_temp_on.set("label", "smart ("+parseFloat(s[18]).toFixed(1)+"°C)");
                            }else{
                                smart_temp_on.set("checked", false);
                                smart_temp_on.set("label", "smart (--.-°C)");
                            }
                            if ( s[3]=="1" ){//Manual mode
                                var moft = parseInt(s[19]);
                                if ( moft != 0 )
                                    manual_off_time.set("label", utils.printDate( moft*1000 ) );
                                else
                                    manual_off_time.set("label", "--:--" );
                                dclass.remove("manual-pane-set", "hidden" );
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
                        
                        html.set("update-time", /*utils.printDate(new Date()) +*/" (ok)" );
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
                    html.set("update-time", /*utils.printDate(new Date()) +*/ " (ko)" );
                    sts.timer = window.setTimeout( function(){ sts.update(); }, 1000 );
                });
            }
        };        
});
