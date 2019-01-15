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
        timer: null,
        wDay: ["lunedi","martedi","mercoledi","giovedi","venerdi", "sabato", "domenica"],
        daySel: null,
        day: 0,
        hour: 0,
        half: 0,
        wDayFixed: [],
        todayH: 10,
        edited: false,
        program: null,
        programUnmod: null,
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
        src: function(p,d,h,f){
            var c = p[d][h*2+f];
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
            prg.refreshProgram();
        },
        nextDay: function(){
            prg.daySel += 1;
            if ( prg.daySel > 6 ) prg.daySel = 0;
            prg.refreshProgram();
        },
		fillDays: function(){
            prg.wDayFixed = [];
            for ( var d = 0; d < 7; d++ ){
                var v = prg.wDay[d];
                if ( d == prg.day ){
                    v += " (oggi)";
                } else if ( d == (prg.day+1)%7 ){
                    v += " (domani)";
                } else if ( d == (prg.day+2)%7 ){
                    v += " (dopodomani)";
                } else if ( d == (prg.day-1)%7 ){
                    v += " (ieri)";
                } else if ( d == (prg.day-2)%7 ){
                    v += " (l'altroieri)";
                }
                prg.wDayFixed[d] = v;
            }
        },
        refreshToday: function(){
            prg.fillDays();
            if ( prg.programUnmod ){
                var tb = Math.max(Math.min( prg.hour-2, 24-prg.todayH ),0);

				for ( var x = 0; x < prg.todayH; x++ ){
					var h = x + tb;
					for ( var f = 0; f < 2; f++ ){
                        var he = dom.byId("today-h-"+x+"-"+f);
                        var ce = dom.byId("today-p-"+x+"-"+f);
                        var cc = query("#today-p-"+x+"-"+f).parent()[0];
                        dclass.remove( he, "now_col" );
                        dclass.remove( cc, "now_col" );
						html.set( he,(h<10?"0"+h:h)+":"+(f*30<10?"0"+(f*30):f*30));
						attr.set( ce, "src", prg.src(prg.programUnmod, prg.day,h,f) );                           
                        if ( h == prg.hour && f == prg.half ){
                            dclass.add( he, "now_col" );
                            dclass.add( cc, "now_col" );
                        }
					}
					var t = x*2+prg.half;
				}
				html.set( dom.byId("today-d"), prg.wDay[ prg.day ] );				
			} else {
				html.set( dom.byId("today-d"), "---" );
			}
		},
        refreshProgram: function(){
            if ( prg.program ){
				html.set( dom.byId("program-d"), prg.wDayFixed[ prg.daySel ] );                
                for ( var h = 0; h < 24; h++ ){
                    for ( var f = 0; f < 2; f++ ){
                        var pi = dom.byId("program-"+h+"-"+f);
                        attr.set( pi, "src", prg.src(prg.program, prg.daySel,h,f) );   
                        dclass.remove( pi, "now_col" );
                        if ( prg.daySel == prg.day ){
                            if ( h == prg.hour ){		
                                if ( f == prg.half ){
                                    dclass.add( pi, "now_col" );
                                }	
                            }
                        }
                    }
                }
			}
		},
        refreshTemplates: function(){
            var opts = [];
            program_template_select.removeOption( program_template_select.getOptions() );
            for ( var t in prg.templates ){
                opts.push( {label: prg.templates[t]["name"], value: t } );
            }
            program_template_select.addOption( opts );
        },
        copyProgram: function(){
            prg.program = [];
            for ( var d = 0; d < 7; d++ ){
                prg.program[d] = [];
                for ( var x = 0; x < prg.programUnmod[d].length; x++ ){
                    prg.program[d][x] = prg.programUnmod[d][x];
                }
            }
        },
        parseProgram: function(result){
            var s = result.split(" ");
            if ( s.length >= 4 ){
                prg.day=parseInt(s[0]);
                prg.hour=parseInt(s[1]);
                prg.half=parseInt(s[2]);
                if ( !prg.daySel ){
                    prg.daySel = prg.day;
                }
                prg.programUnmod = [];
                var x = 0;
                for ( var d = 0; d < 7; d++ ){
                    prg.programUnmod[d] = [];
                    for ( var h = 0; h < 24; h++ ){
                        for ( var f = 0; f < 2; f++ ){
                            prg.programUnmod[d][h*2+f] = s[3][x++];
                        }
                    }
                }
                for ( var t = 4; t < s.length-2; t+= 3){
                    var tn = parseInt(s[t]);
                    prg.templates[tn] = {};
                    prg.templates[tn]["name"] = s[t+1];
                    prg.templates[tn]["value"] = s[t+2];
                }
                prg.refreshToday();
                if ( !prg.edited ){
                    prg.copyProgram();
                    prg.refreshProgram();
                    prg.refreshTemplates();
                }
            }
        },
		update: function(p) {
            if ( prg.timer ){
                window.clearTimeout( prg.timer );
                prg.timer = null;
            }            
            getRequest("cgi-bin/program",                
                function(result){
                    prg.parseProgram(result);
                    prg.timer = window.setTimeout( function(){ prg.update(); }, 60*1000 );
                },
                function(err){
                    prg.timer = window.setTimeout( function(){ prg.update(); }, 60*1000 );
                });
		},
        editTemplate: function(){
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
            prg.copyProgram();
            prg.setEdited(false);
            prg.refreshProgram();
		});
		dialog.show();
	});
	on( prg.apply,"click", function(){
		var dialog = new ConfirmDialog({title:"ATTENZIONE!",content: "Salvare le modifiche?"});
		dialog.set("buttonOk", "Salva");
		dialog.set("buttonCancel", "Continua a modificare");
		dialog.on("execute", function() {
            var p = "program";
            for ( var d = 0; d < 7; d++ ){
                for ( var x = 0; x < prg.program[d].length; x++ ){
                    p += prg.program[d][x];
                }
            }
			postRequest("cgi-bin/program",p,
				function(result){
					prg.setEdited(false);
                    prg.parseProgram(result);
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
            prg.refreshProgram();
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
                prg.refreshProgram();
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
                prg.refreshProgram();
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
                    prg.refreshProgram();
                }
            });
        }
    }
});
