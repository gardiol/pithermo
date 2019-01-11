var prg = null;

require([
    "dojo/dom", 
    "dojo/dom-attr",
    "dojo/dom-class",
    "dojo/dom-style",
    "dojo/dom-construct",
    "dojo/html",
    "dojo/json",
    "dojo/on",
    "dojo/query", 
    "dijit/ConfirmDialog",
    "dijit/form/ToggleButton", 
    "dijit/form/Select",
    "dojo/NodeList-traverse",
    "dojo/domReady!"], 
function( dom, attr, dclass, style, dc, html, json, on, query,  // Dojo
          ConfirmDialog, ToggleButton, Select) // Dijit
{
    prg = {
        wDay: ["lunedi","martedi","mercoledi","giovedi","venerdi", "sabato", "domenica"],
        daySel: 0,
        wDayFixed: [],
        todayBase: 0,
        todayH: 10,
        edited: false,
        program: null,
        selectedType: 'o',
        copyDFrom: null,
        templates: {},

        selOff:    dom.byId("select-off"),
        selGas:    dom.byId("select-gas"),
        selAuto:   dom.byId("select-pelletgas"),
        selPel:    dom.byId("select-pellet"),
        selPelMin: dom.byId("select-pellet-minimum"),
        restore: dom.byId("program-restore"),
        apply:   dom.byId("program-apply"),
        pChange: dom.byId("program-change"),
        
        selectType: function(t){
            prg.selectedType = t;
            [prg.selOff, prg.selGas, prg.selAuto, prg.selPel, prg.selPelMin].forEach(function(o){
                dclass.remove(o,"program-selected");
            });
            if ( t == 'o' ){
                dclass.add(prg.selOff, "program-selected");
            }else if ( t == 'g' ){
                dclass.add(prg.selGas, "program-selected");
            }else if ( t == 'x' ){
                dclass.add(prg.selAuto, "program-selected");
            }else if ( t == 'p' ){
                dclass.add(prg.selPel, "program-selected");
            }else if ( t == 'm' ){
                dclass.add(prg.selPelMin, "program-selected");
            }
        },
        src: function(d,h,f){
            if ( prg.program ){					
                var c = prg.program[d][h*2+f];
                var src = "images/";
                if ( c == 'p' ){
                    src += "pellet.png";
                } else if ( c == 'g' ){
                    src += "gas.png";
                } else if ( c == 'x' ){
                    src += "pellet-gas-1.png";
                } else if ( c == 'm' ){
                    src += "pellet-min-1.png";
                } else {
                    src += "off-1.png";
                }
                return src;
            }
            return "";
        },
        setEdited: function(e){
            prg.edited = e;
            if ( !prg.edited ){
                dclass.add(dom.byId("program-change"), "celated");
                dclass.add(prg.restore, "hidden");
                dclass.add(prg.apply, "hidden");
            }else{
                dclass.remove(dom.byId("program-change"), "celated");
                dclass.remove(prg.restore, "hidden");
                dclass.remove(prg.apply, "hidden");
            }
        },
        toggleEdit: function(){
           dclass.toggle(dom.byId("program-editor"), "hidden");
        },
        prevDay: function(){
            prg.daySel -= 1;
            if ( prg.daySel < 0 ) prg.daySel = 6;
            prg.refresh();
        },
        nextDay: function(){
            prg.daySel += 1;
            if ( prg.daySel > 6 ) prg.daySel = 0;
            prg.refresh();
        },
		fillDays: function(w){
            prg.wDayFixed = [];
            for ( var d = 0; d < 7; d++ ){
                var v = prg.wDay[d];
                if ( d == w ){
                    v += " (oggi)";
                } else if ( d == (w+1)%7 ){
                    v += " (domani)";
                } else if ( d == (w+2)%7 ){
                    v += " (dopodomani)";
                } else if ( d == (w-1)%7 ){
                    v += " (ieri)";
                } else if ( d == (w-2)%7 ){
                    v += " (l'altroieri)";
                }
                prg.wDayFixed[d] = v;
            }
        },
        refresh: function(){
            if ( sts.status && prg.program ){
                var n = sts.status.now;
				prg.fillDays(n.d);
                
				for ( var x = 0; x < prg.todayH; x++ ){
					var h = x + prg.todayBase;
					for ( var f = 0; f < 2; f++ ){
                        var he = dom.byId("today-h-"+x+"-"+f);
                        var ce = dom.byId("today-p-"+x+"-"+f);
                        dclass.remove( he, "now_col" );
                        dclass.remove( ce, "now_col" );
						html.set( he,(h<10?"0"+h:h)+":"+(f*30<10?"0"+(f*30):f*30));
						attr.set( ce, "src", prg.src(n.d,h,f) );                           
                        if ( h == n.h && f == n.f ){
                            dclass.add( he, "now_col" );
                            dclass.add( ce, "now_col" );
                        }
					}
					var t = x*2+n.f;
				}
				html.set( dom.byId("today-d"), prg.wDay[ sts.status.now.d ] );
				
				html.set( dom.byId("program-d"), prg.wDayFixed[ prg.daySel ] );
                
                for ( var h = 0; h < 24; h++ ){
                    for ( var f = 0; f < 2; f++ ){
                        var pi = dom.byId("program-"+h+"-"+f);
                        attr.set( pi, "src", prg.src(prg.daySel,h,f) );   
                        dclass.remove( pi, "now_col" );
                        if ( prg.daySel == n.d ){
                            if ( h == n.h ){		
                                if ( f == n.f ){
                                    dclass.add( pi, "now_col" );
                                }	
                            }
                        }
                    }
                }
			} else {
				html.set( dom.byId("today-d"), "---" );
			}
		},
		update: function(p) {
			if ( !prg.edited ){
				if ( sts.status )
				   prg.daySel = sts.status.now.d;
				prg.program = [];
				for ( var d = 0; d < p.length; d++ ){
					prg.program[d] = [];
					for ( var h = 0; h < p[d].length; h++ )                                
						prg.program[d][h] = p[d][h];
				}
				prg.setEdited(false);
			}
			if ( sts.status ){
				prg.todayBase = Math.max(Math.min( sts.status.now.h-2, 24-prg.todayH ),0);
				prg.refresh();
			}
		},
        
        
        
	};


	on(prg.selOff, "click", function(){prg.selectType('o');});
	on(prg.selGas, "click", function(){prg.selectType('g');});
	on(prg.selAuto, "click", function(){prg.selectType('x');});
	on(prg.selPel, "click", function(){prg.selectType('p');});
	on(prg.selPelMin, "click", function(){prg.selectType('m');});
	prg.selectType('o');
        
	on( prg.restore,"click", function(){
		var dialog = new ConfirmDialog({title: "ATTENZIONE!", content: "Annullare le modifiche?"});
		dialog.set("buttonOk", "Si, annulla");
		dialog.set("buttonCancel", "No, continua");
		dialog.on("execute", function() {
			if ( sts.status ) {
                prg.setEdited(false);
				prg.update(sts.status.program);
			}
		});
		dialog.show();
	});
	on( prg.apply,"click", function(){
		var dialog = new ConfirmDialog({title:"ATTENZIONE!",content: "Salvare le modifiche?"});
		dialog.set("buttonOk", "Salva");
		dialog.set("buttonCancel", "Continua a modificare");
		dialog.on("execute", function() {
			var ps = json.stringify(prg.program);
			postRequest("cgi-bin/program",{json: prg.program},
				function(result){
					prg.setEdited(false);
				},
				function(err){alert("Command error: " + err );});
		});
		dialog.show();
	});

    on( query("#program_copy").parent()[0], "click", function(evt){
        if ( prg.program ){
            if ( prg.copyDFrom === null ){
                prg.copyDFrom = prg.daySel;
                attr.set(dom.byId("program_copy"),  "src", "images/paste.png");
            } else {
                attr.set(dom.byId("program_copy"),  "src", "images/copy.png");
                if ( prg.daySel != prg.copyDFrom ){
                    for ( var n = 0; n < prg.program[prg.daySel].length; n++ )
                        prg.program[prg.daySel][n] = prg.program[prg.copyDFrom][n]; 	
                    prg.copyDFrom = null;
                    prg.setEdited(true);
                }
            }	
            prg.refresh();
        }
    });	

    on( query("#program-d")[0], "click", function(evt){
        if ( prg.program ){
            var dialog = new ConfirmDialog({title: "Imposta giornata",
                                            content: "Imposto l'intero giorno?"});
            dialog.set("buttonOk", "Si");
            dialog.set("buttonCancel", "Annulla");
            dialog.on("execute",function() {
                for ( var h = 0; h < 24; h++ ){
                    for ( var f = 0; f < 2; f++ ){
                        prg.program[prg.daySel][h*2+f] = prg.selectedType;
                    }
                }
                prg.setEdited(true);
                prg.refresh();
            });
            dialog.show();
        }
    });

    on( query("#program_clear").parent()[0], "click", function(evt){
        if ( prg.program ){
            var dialog = new ConfirmDialog({title: "Resetto giornata",
                                              content: "Resetto l'intero giorno?"});
            dialog.set("buttonOk", "Si");
            dialog.set("buttonCancel", "Annulla");
            dialog.on("execute",function() {
                for ( var h = 0; h < 24; h++ ){
                    for ( var f = 0; f < 2; f++ ){
                        prg.program[prg.daySel][h*2+f] = 'o';
                    }
                }
                prg.setEdited(true);
                prg.refresh();
              });
              dialog.show();
          }
    });

    for ( var h = 0; h < 24; h++ ){
        for ( var f = 0; f < 2; f++ ){
            let h2 = h;
            let f2 = f;
            on( query("#program-"+h+"-"+f).parent()[0], "click", function(evt){
                if ( prg.program ){
                    var x = h2*2+f2;
                    prg.setEdited(true);
                    prg.program[prg.daySel][x] = prg.program[prg.daySel][x] == prg.selectedType ? 'o' : prg.selectedType;
                    prg.refresh();
                }
            });
        }
    }
});
