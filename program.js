var prg = null;
var week_day = ["lunedi","martedi","mercoledi","giovedi","venerdi", "sabato", "domenica"];
var week_day_short = ["LU","MA","ME","GI","VE", "SA", "DO"];

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
    "dijit/form/CheckBox",
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
function( request, dom, attr, dclass, style, dc, html, query, json, on, win,     // Dojo
          registry, ConfirmDialog, ContentPane, TabContainer, Button, ToggleButton, CheckBox, NumberSpinner, Select, HorizontalSlider, // Dijit
          Chart, Default, Lines, Chris, Areas, Markers, MouseIndicator )               // Charing
{
	
    prg = {
		restore: dc.create("img", { src: "images/restore.png", class:"corner hidden" } ),
		apply: dc.create("img", { src: "images/apply.png", class:"corner hidden" } )
    };
	
	
	    var tdr = [];
	var prgrf = [];
    var program_status = null;
    var selected_type;
    var copyInProgressD = null;
    var copyInProgressH = null;
    var programEdited = false;

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
        for ( var d = 0; (d < 7) && !programEdited; d++ )
            for ( var h = 0; (h < 48) && !programEdited; h++ )
                if ( program_status[d][h] != system_status.program[d][h] )
                    programEdited = true;
        if ( !programEdited ){
            dclass.add(dom.byId("program-change"), "celated");
			dclass.add(prg.restore, "hidden");
			dclass.add(prg.apply, "hidden");
        }else{
            dclass.remove(dom.byId("program-change"), "celated");
			dclass.remove(prg.restore, "hidden");
			dclass.remove(prg.apply, "hidden");
        }

		for ( var h = 0; h < 24; h++ ){
			dclass.remove(tdr[h]["h"], "auto_now_c" );
			dclass.remove(tdr[h]["c"], "auto_now_c" );
			dclass.remove(prgrf["hdr_h"][h], "auto_now_c" );
			dclass.remove(prgrf["hdr_ch"][h], "auto_now_c" );
			for ( var f = 0; f < 2; f++ ){
				dclass.remove(prgrf["hdr_f"][h][f], "auto_now_c" );
			}
		}
		for ( var d = 0; d < 7; d++ ){
			dclass.remove(prgrf["hdr_d"][d], "auto_now_c" );
			dclass.remove(prgrf["hdr_cd"][d], "auto_now_c" );
		}
		var today_base = system_status.now.h - 6;
		if ( today_base < 0 )
			today_base = 0;
		if ( today_base > 12 )
			today_base = 12;
		for ( var h = 0; h < 12; h++ ){
			var rh = h + today_base;
			html.set(tdr[h*2]["h"], (rh < 10 ? "0"+rh:rh)+":00" );
			html.set(tdr[h*2+1]["h"], (rh < 10 ? "0"+rh:rh)+":30" );
		}
		html.set(tdr["day"], week_day[ system_status.now.d ] );
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
                    attr.set(prgrf["cell"][d][h][f]["_img"], "src", src );   
                    
                    dclass.remove(prgrf["cell"][d][h][f], "auto_now_now" )                                
                    dclass.remove(prgrf["cell"][d][h][f], "auto_now_c" );
                    if ( d == system_status.now.d ){
						var today_cell = (h - today_base)*2 + f;
						if ( (today_cell < 0) || (today_cell > 23 ) )
							today_cell = null;
						if ( today_cell )
							attr.set(tdr[today_cell]["img"], "src", src );
                        if ( h == system_status.now.h ){
                            if ( f == system_status.now.f ){
								dclass.add(prgrf["hdr_h"][h], "auto_now_c" );
								dclass.add(prgrf["hdr_ch"][h], "auto_now_c" );
								dclass.add(prgrf["hdr_d"][d], "auto_now_c" );
								dclass.add(prgrf["hdr_cd"][d], "auto_now_c" );
								dclass.add(prgrf["hdr_f"][h][f], "auto_now_c" );
                                dclass.add(prgrf["cell"][d][h][f], "auto_now_now" );                         
								dclass.add(tdr[today_cell]["c"], "auto_now_c" );
								dclass.add(tdr[today_cell]["h"], "auto_now_c" );
                            } else {
                                dclass.add(prgrf["cell"][d][h][f], "auto_now_c" );     
                            }
                        }
                        else
                            dclass.add(prgrf["cell"][d][h][f], "auto_now_c" );                         
                    } else {
                        if ( h == system_status.now.h ){
                            if ( f == system_status.now.f ){
                                dclass.add(prgrf["cell"][d][h][f], "auto_now_c" );                         
							}
						}
					}
                }
            }
        }
	}



    function buildProgram(){
        new ToggleButton({ checked: false, onChange: function(v) {
            if ( v ){
                dclass.remove(dom.byId("program-editor"), "hidden");
            } else {
                dclass.add(dom.byId("program-editor"), "hidden");
            }
        }}, "program-size");		
		
        on(dom.byId("select-off"), "click", function(){selectType('o');});
        on(dom.byId("select-gas"), "click", function(){selectType('g');});
        on(dom.byId("select-pelletgas"), "click", function(){selectType('x');});
        on(dom.byId("select-pellet"), "click", function(){selectType('p');});
        on(dom.byId("select-pellet-minimum"), "click", function(){selectType('m');});
        selectType('o');
                        		
        dc.empty(dom.byId("program-table"));
		var ptable = dom.byId("program-table");
		dc.create("col", {colspan:2}, ptable );
        for ( var h = 0; h < 48; h++ ){
            dc.create("col", {class: h%2 ? "halfCol" : "hourCol"}, ptable );                 
		}
		var ch_row = dc.create("tr", null, ptable );
		var tl_corner = dc.create("th", {colspan:2,rowspan:3}, ch_row );
		var ph_row = dc.create("tr", null, ptable );
		var pf_row = dc.create("tr", { class: "tr_br" }, ptable );
		
		
		dc.place( prg.apply, dc.create("div", null, tl_corner) );
		dc.place( prg.restore, dc.create("div", null, tl_corner) );
		on( prg.restore,"click", function(){
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
		});
		
		on( prg.apply,"click", function(){
			var dialog = new ConfirmDialog({
				title: "ATTENZIONE!",
				content: "Salvare le modifiche?"});
			dialog.set("buttonOk", "Salva");
			dialog.set("buttonCancel", "Continua a modificare");
			dialog.on("execute", function() {
				var ps = json.stringify(program_status);
				postRequest("cgi-bin/program",{json: program_status},
					function(result){
						programEdited = false;
						dclass.add(prg.restore, "hidden");
						dclass.add(prg.apply, "hidden");
					},
					function(err){alert("Command error: " + err );});
			});
			dialog.show();
		});

		prgrf["hdr_ch"] = [];
		prgrf["hdr_h"] = [];
		prgrf["hdr_f"] = [];
		for (var h = 0; h < 24; h++ ){
			var h_str = h < 10 ? "0"+h : ""+h;
			var copy_h = prgrf["hdr_ch"][h] = dc.create("td", {colspan:2}, ch_row );
			prgrf["hdr_ch"][h]["_h"] = h;
			prgrf["hdr_ch"][h]["_img"] = dc.create("img", { class:"copy", src: "images/copy.png" }, copy_h );            
            on(copy_h, "click", function(evt){
                if ( program_status ){
                    var h = evt.currentTarget._h;
                    if ( copyInProgressH === null ){
                        copyInProgressH = h;
                        for ( var x = 0; x < 24; x++ )
                            attr.set(prgrf["hdr_ch"][x]["_img"], "src", (x == copyInProgressH) ? "images/cancel_copy.png" : "images/paste.png");
                    } else {
                        for ( var x = 0; x < 24; x++ )
                            attr.set(prgrf["hdr_ch"][x]["_img"], "src", "images/copy.png" );
                        for ( var d = 0; d < 7; d++ ){
                            program_status[d][h*2] = program_status[d][copyInProgressH*2]; 
                            program_status[d][h*2+1] = program_status[d][copyInProgressH*2+1]; 
                        }
                        copyInProgressH = null;
                    }	
                    programRefresh();
                }
            });						
			prgrf["hdr_h"][h] = dc.create("th", {innerHTML: h_str, colspan:2}, ph_row );
			prgrf["hdr_h"][h]["_h"] = h;
            on(prgrf["hdr_h"][h], "click", function(evt){
                if ( program_status ){
                    var h = evt.currentTarget._h;
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
			prgrf["hdr_f"][h] = [];
			for (var f = 0; f < 2; f++){
				prgrf["hdr_f"][h][f] = dc.create("th", {innerHTML: f == 0 ? "00":"30",class:"smallNo"}, pf_row ); 				
			}
		}
		
		prgrf["hdr_d"] = [];
		prgrf["hdr_cd"] = [];
		prgrf["cell"] = [];
		for ( var d = 0; d < 7; d++ ){
			var row = dc.create("tr", { class: "tr_br" }, ptable );
			var copy_d = prgrf["hdr_cd"][d] = dc.create("td", null, row );			
            prgrf["hdr_cd"][d]["_d"] = d;
            prgrf["hdr_cd"][d]["_img"] = dc.create("img", { class:"copy", src: "images/copy.png" }, copy_d );            
            on(copy_d, "click", function(evt){
                if ( program_status ){
                    var d = evt.currentTarget._d;
                    if ( copyInProgressD === null ){
                        copyInProgressD = d;
                        for ( var x = 0; x < 7; x++ )
                            attr.set(prgrf["hdr_cd"][x]["_img"], "src", (x == copyInProgressD) ? "images/cancel_copy.png" : "images/paste.png");
                    } else {
                        for ( var x = 0; x < 7; x++ )
                            attr.set(prgrf["hdr_cd"][x]["_img"], "src", "images/copy.png" );
                        for ( var n = 0; n < program_status[d].length; n++ )
                            program_status[d][n] = program_status[copyInProgressD][n]; 	
                        copyInProgressD = null;
                    }	
                    programRefresh();
				}
            });
			prgrf["hdr_d"][d] = dc.create("th", { innerHTML:week_day_short[d] }, row );
			prgrf["hdr_d"][d]["_d"] = d;
            on(prgrf["hdr_d"][d], "click", function(evt){
                if ( program_status ){
                    var d = evt.currentTarget._d;
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
			prgrf["cell"][d] = [];
			for ( var h = 0; h < 24; h++ ){
				prgrf["cell"][d][h] = [];
				for ( var f = 0; f < 2; f++ ){
					prgrf["cell"][d][h][f] = dc.create("td", null, row );	
                    prgrf["cell"][d][h][f]["_img"] = dc.create("img", { src: "images/off.png" }, prgrf["cell"][d][h][f] );            
                    prgrf["cell"][d][h][f]["_d"] = d;
                    prgrf["cell"][d][h][f]["_h"] = h;
                    prgrf["cell"][d][h][f]["_f"] = f;
                    on(prgrf["cell"][d][h][f], "click", function(evt){
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

		var table = dom.byId("today-table");
        for ( var h = 0; h < 24; h++ ){
            dc.create("col", {class: h%2 == 0? "halfCol" : "hourCol"}, table );                 
		}
        var top_row = dc.create("tr", null, table );
		tdr["day"] = dc.create("th", { colspan: 24, innerHTML: "..." }, top_row );
		var h_row = dc.create( "tr", { class: "solid-down-sep" }, table );
		var c_row = dc.create( "tr", { class: "solid-down-sep" }, table );
        for ( var h = 0; h < 24; h++ ){
			tdr[h] = [];
			tdr[h]["h"] = dc.create("td", { innerHTML: ""}, h_row );                 
			tdr[h]["c"] = dc.create("td", { innerHTML: ""}, c_row );                 
			tdr[h]["img"] = dc.create("img", { src: "images/off.png" }, tdr[h]["c"] );            
		}		
	}	
    


prg.update: function() {
				if ( !programEdited ){
					program_status = [];
					for ( var d = 0; d < sts.status.program.length; d++ ){
						program_status[d] = [];
						for ( var h = 0; h < sts.status.program[d].length; h++ )                                
							program_status[d][h] = sts.status.program[d][h];
					}
					programRefresh();
				}
}