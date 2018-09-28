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
    "dijit/ConfirmDialog",
    "dijit/form/ToggleButton", 
    "dijit/form/Select",
    "dojo/domReady!"], 
function( dom, attr, dclass, style, dc, html, json, on,     // Dojo
          ConfirmDialog, ToggleButton, Select) // Dijit
{
	
    prg = {
		wDay: ["lunedi","martedi","mercoledi","giovedi","venerdi", "sabato", "domenica"],
		restore: dom.byId("program-restore"),
		apply:   dom.byId("program-apply"),
      edited: false,
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
      program: null,
      copyDFrom: null,
      todayBase: 0,
      todayH: 8,
      todayT: {},
      programT: {},
      selOff:    dom.byId("select-off"),
      selGas:    dom.byId("select-gas"),
      selAuto:   dom.byId("select-pelletgas"),
      selPel:    dom.byId("select-pellet"),
      selPelMin: dom.byId("select-pellet-minimum"),
      pChange: dom.byId("program-change"),
      daySel: new Select({onChange: function(v){
      	for ( var d = 0; d < 7; d++ ){
      		if ( d == v ){      			
	      		dclass.remove(prg.programT[d]["table"], "hidden");
	      	} else {
	      		dclass.add(prg.programT[d]["table"], "hidden");
	      	}
      	}
		}},"program-day-select"), 
		selectedType: 'o',
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
		update: function(p) {
			if ( !prg.edited ){
				prg.program = [];
				for ( var d = 0; d < p.length; d++ ){
					prg.program[d] = [];
					for ( var h = 0; h < p[d].length; h++ )                                
						prg.program[d][h] = p[d][h];
				}
				prg.setEdited(false);
			}
			if ( sts.status ){
				prg.todayBase = Math.max(Math.min( sts.status.now.h-prg.todayH/2, 24-prg.todayH ),0);
				prg.refresh();
			}
		},
		srcCalc: function(d,h,f){
			if ( prg.program ){					
				var c = prg.program[d][h*2+f];
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
				return src;
			}
			return "";
		},
		fillDaysFirst: true,
		fillDays: function(w){
			var old_d = prg.daySel.get("value");
			prg.daySel.removeOption( prg.daySel.getOptions() );
			for ( var d = 0; d < 7; d++ ){
				var v = prg.wDay[d];
				var x = "";
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
				prg.daySel.addOption( { value:""+d, label:""+v+x, selected: (d==w) } );
				html.set(prg.programT[d]["day"], v );
			}
			prg.daySel.setValue( prg.fillDaysFirst ? w : old_d );
			prg.fillDaysFirst = false;
		},
		refresh: function(){
			if ( sts.status ){
				var n = sts.status.now;
				prg.fillDays(n.d);
				for ( var x = 0; x < prg.todayH; x++ ){
					var h = x + prg.todayBase;		
					for ( var f = 0; f < 2; f++ ){
						html.set(prg.todayT[x*2+f]["_h"],(h<10?"0"+h:h)+":"+(f*30<10?"0"+(f*30):f*30));
						attr.set(prg.todayT[ x*2+f ]["_img"], "src", prg.srcCalc(n.d,h,f) );   
					}
					var t = x*2+n.f;
					dclass.remove(prg.todayT[t]["_c"], "now_col" );
					dclass.remove(prg.todayT[t]["_h"], "now_col" );									
					if ( h == n.h ){
						dclass.add(prg.todayT[t]["_c"], "now_col" );
						dclass.add(prg.todayT[t]["_h"], "now_col" );									
					}
				}
					
				if ( prg.program ){
					for ( var d = 0; d < 7; d++ ){
						for ( var h = 0; h < 24; h++ ){
							for ( var f = 0; f < 2; f++ ){
								attr.set(prg.programT[d][h][f]["_img"], "src", prg.srcCalc(d,h,f) );   
								dclass.remove( prg.programT[d][h][f], "now_col" );
								if ( d == n.d ){
									if ( h == n.h ){		
										if ( f == n.f ){
											dclass.add( prg.programT[d][h][f], "now_col" );
										}	
									}
								}
							}
						}
					}
				}
				html.set(prg.todayT["day"], prg.wDay[ sts.status.now.d ] );
			} else {
				html.set(prg.todayT["day"], "---" );			
			}
		},
	};

   new ToggleButton({ checked: false, onChange: function(v) {
       if ( v ){
           dclass.remove(dom.byId("program-editor"), "hidden");
       } else {
           dclass.add(dom.byId("program-editor"), "hidden");
       }
   }}, "program-size");		

	on(prg.selOff, "click", function(){prg.selectType('o');});
	on(prg.selGas, "click", function(){prg.selectType('g');});
	on(prg.selAuto, "click", function(){prg.selectType('x');});
	on(prg.selPel, "click", function(){prg.selectType('p');});
	on(prg.selPelMin, "click", function(){prg.selectType('m');});
	prg.selectType('o');
        
	on( prg.restore,"click", function(){
		var dialog = new ConfirmDialog({
				title: "ATTENZIONE!",
				content: "Annullare le modifiche?"});
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
		var dialog = new ConfirmDialog({
			title: "ATTENZIONE!",
			content: "Salvare le modifiche?"});
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

	for ( var d = 0; d < 7; d++ ){
		prg.programT[d] = [];
		prg.programT[d]["table"] = dc.create("table", {class:"program-table hidden"}, dom.byId("program-table"));
		for ( var x = 0; x < 3; x++ ){			
	      dc.create("col", {class:"hourCol"}, prg.programT[d]["table"] );                 
	      dc.create("col", {class:"halfCol"}, prg.programT[d]["table"] );                 
	      dc.create("col", {class:"hourCol" }, prg.programT[d]["table"] );                 
	      if ( x != 2 )
		      dc.create("col", {class:"hourCol"}, prg.programT[d]["table"] );
		}		      
		var dr = dc.create("tr", {class: "header-row"}, prg.programT[d]["table"] );
		prg.programT[d]["copy"] = dc.create("th", null, dr );
		prg.programT[d]["copy"]["_d"] = d;
		prg.programT[d]["copy"]["_img"] = dc.create("img", { class:"copy", src: "images/copy.png" }, prg.programT[d]["copy"] );
		on(prg.programT[d]["copy"], "click", function(evt){
			if ( prg.program ){
			    var d = evt.currentTarget._d;
			    if ( prg.copyDFrom === null ){
			        prg.copyDFrom = d;
			        for ( var x = 0; x < 7; x++ )
			            attr.set(prg.programT[x]["copy"]["_img"], "src", (x==d) ? "images/cancel_copy.png" : "images/paste.png");
			    } else {
			        for ( var x = 0; x < 7; x++ )
			            attr.set(prg.programT[x]["copy"]["_img"], "src", "images/copy.png" );
			        for ( var n = 0; n < prg.program[d].length; n++ )
			            prg.program[d][n] = prg.program[prg.copyDFrom][n]; 	
			        prg.copyDFrom = null;
						prg.setEdited(true);
			    }	
			    prg.refresh();
			}
		});	
		prg.programT[d]["day"] = dc.create("th", { colspan: 9 }, dr );
		prg.programT[d]["day"]["_d"] = d;
		on(prg.programT[d]["day"], "click", function(evt){
          if ( prg.program ){
              var d = evt.currentTarget._d;
              var dialog = new ConfirmDialog({title: "Imposta giornata",
                                              content: "Imposto l'intero giorno?"});
              dialog.set("buttonOk", "Si");
              dialog.set("buttonCancel", "Annulla");
              dialog.on("execute",function() {
                  for ( var h = 0; h < 24; h++ ){
                      for ( var f = 0; f < 2; f++ ){
                          prg.program[d][h*2+f] = prg.selectedType;
                      }
                  }
						prg.setEdited(true);
                  prg.refresh();
              });
              dialog.show();
          }
      });
		prg.programT[d]["extr"] = dc.create("th", null, dr );
		prg.programT[d]["extr"]["_d"] = d;
		prg.programT[d]["extr"]["_img"] = dc.create("img", { class:"copy", src: "images/restore.png" }, prg.programT[d]["extr"] );
		on(prg.programT[d]["extr"], "click", function(evt){
          if ( prg.program ){
              var d = evt.currentTarget._d;
              var dialog = new ConfirmDialog({title: "Resetto giornata",
                                              content: "Resetto l'intero giorno?"});
              dialog.set("buttonOk", "Si");
              dialog.set("buttonCancel", "Annulla");
              dialog.on("execute",function() {
                  for ( var h = 0; h < 24; h++ ){
                      for ( var f = 0; f < 2; f++ ){
                          prg.program[d][h*2+f] = 'o';
                      }
                  }
						prg.setEdited(true);
                  prg.refresh();
              });
              dialog.show();
          }
      });
		var tr = dc.create("tr", null, prg.programT[d]["table"] );
		dc.create("th", null, tr );
		for ( var x = 0; x < 3; x++ ){
			dc.create("th", { innerHTML: "00" }, tr );
			dc.create("th", { innerHTML: "30" }, tr );
			if ( x != 2 )
				dc.create("th", { colspan: 2 }, tr );
		}
		for ( var h1 = 0; h1 < 8; h1++ ){
			var hr = dc.create("tr", null, prg.programT[d]["table"] );
			for ( var h2 = 0; h2 < 3; h2++ ){
				var h = h1+h2*8;	
				prg.programT[d][h] = [];
				prg.programT[d][h]["h"] = dc.create("th", { innerHTML: h < 10 ? "0"+h:h}, hr );
				for ( var f = 0; f < 2; f++ ){
					prg.programT[d][h][f] = dc.create("td", null, hr );            
					prg.programT[d][h][f]["_img"] = dc.create("img", { src: "images/off.png" }, prg.programT[d][h][f] );  
					prg.programT[d][h][f]["_img"]["_d"] = d;        
					prg.programT[d][h][f]["_img"]["_h"] = h;        
					prg.programT[d][h][f]["_img"]["_f"] = f;
					on(prg.programT[d][h][f]["_img"], "click", function(evt){
                  if ( prg.program ){
							var i = evt.currentTarget;
							var x = i._h*2+i._f;
							prg.setEdited(true);
							prg.program[i._d][x] = prg.program[i._d][x] == prg.selectedType ? 'o' : prg.selectedType;
							prg.refresh();
						}
					});          							
				}
				if ( h2 != 2 )
					dc.create("td", { class: "spacer" }, hr );            					
			}
		}
  	}
 
	prg.todayT["table"] = dc.create("table", {class:"program-table"}, dom.byId("today-table-div"));
     for ( var h = 0; h < prg.todayH*2; h++ ){
         dc.create("col", {class: h%2 == 0? "halfCol" : "hourCol"}, prg.todayT["table"] );                 
	}
	var top_row = dc.create("tr", null, prg.todayT["table"] );
	prg.todayT["day"] = dc.create("th", { colspan: 24, innerHTML: "..." }, top_row );
	var h_row = dc.create( "tr", { class: "header-row" }, prg.todayT["table"] );
	var c_row = dc.create( "tr", {}, prg.todayT["table"] );
	for ( var h = 0; h < prg.todayH*2; h++ ){
		prg.todayT[h] = [];
		prg.todayT[h]["_h"] = dc.create("td", { innerHTML: ""}, h_row );                 
		prg.todayT[h]["_c"] = dc.create("td", { innerHTML: ""}, c_row );                 
		prg.todayT[h]["_img"] = dc.create("img", { class: "today-table-img", src: "images/off.png" }, prg.todayT[h]["_c"] );            
	}

});
