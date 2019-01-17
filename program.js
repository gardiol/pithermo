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
        program: {},
        programUnmod: null,
        selectedType: 'o',
        copyDFrom: null,
        templates: {},    
        num_templates: 0,

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
        src: function(p,h,f){
            var c = p[h*2+f];
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
            if ( prg.daySel >= 0 )
                html.set(dom.byId("program-changed"), "Salvare il programma modificato?");
            else
                html.set(dom.byId("program-changed"), "Salvare il template modificato?");
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
        setEditTemplate: function(e){
            
        },
        toggleEdit: function(){
           dclass.toggle(dom.byId("program-editor"), "hidden");
        },
        prevDay: function(){
            if ( prg.daySel >= 0){
                prg.daySel -= 1;
                if ( prg.daySel < 0 ) prg.daySel = 6;
                prg.refreshProgram();
            }
        },
        nextDay: function(){
            if ( prg.daySel >= 0){
                prg.daySel += 1;
                if ( prg.daySel > 6 ) prg.daySel = 0;
                prg.refreshProgram();
            }
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
						attr.set( ce, "src", prg.src(prg.programUnmod[prg.day],h,f) );                           
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
            html.set( dom.byId("program-d"), prg.daySel >= 0 ? prg.wDayFixed[ prg.daySel ] : prg.templates[-(prg.daySel+1)] );
            for ( var h = 0; h < 24; h++ ){
                for ( var f = 0; f < 2; f++ ){
                    var pi = dom.byId("program-"+h+"-"+f);
                    attr.set( pi, "src", prg.src( prg.program[prg.daySel],h,f) );   
                    dclass.remove( pi, "now_col" );
                    if ( Math.abs(prg.daySel) == prg.day ){
                        if ( h == prg.hour ){		
                            if ( f == prg.half ){
                                dclass.add( pi, "now_col" );
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
                opts.push( {label: prg.templates[t], value: t } );
            }
            program_template_select.addOption( opts );
        },
        copyProgram: function(){
            for ( var d = -(prg.num_templates+1); d < 7; d++ ){
                prg.program[d] = prg.programUnmod[d];
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
                    prg.programUnmod[d] = s[3].substr( d*48, 48 );
                }
                for ( var t = 4; t < s.length-2; t+= 3){
                    var tn = parseInt(s[t]);
                    prg.templates[tn] = s[t+1];
                    prg.programUnmod[-(tn+1)] = s[t+2];
                    if ( prg.num_templates < tn )
                        prg.num_templates = tn;
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
            if ( !prg.edited ){
                if ( prg.daySel >= 0 ){
                    dclass.add(dom.byId("program-table"), "edit-template");
                    program_template_name.set("disabled", true);
                    program_template_set.set("disabled", true);
                    program_template_edit.set("label", "Chiudi");
                    program_template_select.set("disabled", true );
                    dclass.add(program_prev_day.domNode, "hidden");
                    dclass.add(program_next_day.domNode, "hidden");
                    dclass.add( dom.byId("program_copy"), "hidden");
                    dclass.add( dom.byId("program_clear"), "hidden");
                    var v = program_template_select.get("value");
                    prg.daySel = -(v+1);                
                    prg.refreshProgram();                
                } else
                    prg.uneditTemplate();
            }else
                alert("Prima salvare o annullare le modifiche");
        },
        uneditTemplate: function(){
            dclass.remove(dom.byId("program-table"), "edit-template");
            program_template_name.set("disabled", false);
            program_template_set.set("disabled", false);
            program_template_edit.set("label", "Modifica...");
            program_template_select.set("disabled", false );
            dclass.remove(program_prev_day.domNode, "hidden");
            dclass.remove(program_next_day.domNode, "hidden");
            dclass.remove( dom.byId("program_copy"), "hidden");
            dclass.remove( dom.byId("program_clear"), "hidden");
            prg.daySel = prg.day;
            prg.setEdited(false);
            prg.refreshProgram();   
        },
        applyTemplate: function(){
            if ( prg.daySel >= 0 ){
                var dialog = new ConfirmDialog({title: "ATTENZIONE!", content: "Applico il template?"});
                dialog.set("buttonOk", "Si, applica");
                dialog.set("buttonCancel", "Annulla");
                dialog.on("execute", function() {
                    var v = program_template_select.get("value");
                    prg.program[prg.daySel] = prg.program[-(v+1)];
                    prg.refreshProgram();
                    prg.setEdited(true);
                });
                dialog.show();
            } else
                alert("Stai già modificando un template");
        },
        saveName: function(n){
            var v = program_template_select.get("value");
            n = n.replace(/\s+/g, '');
            var p = "template-set"+v+":"+n+":"+prg.program[-(v+1)];
			postRequest("cgi-bin/program",p,
				function(result){
					prg.setEdited(false);
                    prg.parseProgram(result);
				},
				function(err){alert("Command error: " + err );});            
        },
        nameTemplate: function(){
            var v = program_template_select.get("value");
            ptNewName.set("value", prg.templates[v] );
            ptName.show();
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
            if ( prg.daySel >= 0 ){
                var p = "program";
                for ( var d = 0; d < 7; d++ ){
                    p += prg.program[d];
                }
                postRequest("cgi-bin/program",p,
                    function(result){
                        prg.setEdited(false);
                        prg.parseProgram(result);
                    },
                    function(err){alert("Command error: " + err );});
            } else {
                var t = -(prg.daySel+1);
                var p = "template-set" + t + ":" + prg.templates[t] + ":" + prg.program[prg.daySel];
                postRequest("cgi-bin/program",p,
                    function(result){
                        prg.setEdited(false);
                        prg.parseProgram(result);
                    },
                    function(err){alert("Command error: " + err );});
            }
		});
		dialog.show();
	});

    on( query("#program_copy").parent()[0], "click", function(evt){
        if ( prg.daySel > 0 ){
            if ( prg.copyDFrom === null ){
                prg.copyDFrom = prg.daySel;
                attr.set(dom.byId("program_copy"),  "src", "images/paste.png");
            } else {
                attr.set(dom.byId("program_copy"),  "src", "images/copy.png");
                if ( prg.daySel != prg.copyDFrom ){
                    prg.program[prg.daySel] = prg.program[prg.copyDFrom]; 	
                    prg.copyDFrom = null;
                    prg.setEdited(true);
                }
            }	
            prg.refreshProgram();
        }
    });	

    on( query("#program-d")[0], "click", function(evt){
        if ( prg.program ){
            var t = prg.daySel >= 0 ? "gioranata" : "template";
            var dialog = new ConfirmDialog({title: "Imposta "+t,
                                            content: "Imposto l'intera "+t+"?"});
            dialog.set("buttonOk", "Si");
            dialog.set("buttonCancel", "Annulla");
            dialog.on("execute",function() {
                prg.program[prg.daySel] = "";
                for ( var x = 0; x < 48; x++ ){
                    prg.program[prg.daySel] += prg.selectedType;
                }
                prg.setEdited(true);
                prg.refreshProgram();
            });
            dialog.show();
        }
    });

    on( query("#program_clear").parent()[0], "click", function(evt){
        if ( prg.program ){
            var t = prg.daySel >= 0 ? "gioranata" : "template";
            var dialog = new ConfirmDialog({title: "Resetto "+t,
                                              content: "Resetto l'intera "+t+"?"});
            dialog.set("buttonOk", "Si");
            dialog.set("buttonCancel", "Annulla");
            dialog.on("execute",function() {
                prg.program[prg.daySel] = "";
                for ( var x = 0; x < 48; x++ ){
                    prg.program[prg.daySel] += 'o';
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
                    var c = prg.program[prg.daySel].charAt(x) == prg.selectedType ? 'o' : prg.selectedType;
                    prg.program[prg.daySel] = prg.program[prg.daySel].slice(0,x)+c+prg.program[prg.daySel].slice(x+1);
                    prg.refreshProgram();
                }
            });
        }
    }
});
