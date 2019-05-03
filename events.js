var evt = null;
 
require([
    "dojo/dom", 
    "dojo/dom-class",
    "dojo/on",
    "dojo/dom-construct",
    "dojo/domReady!"], 
function( dom, dclass, on, dc)    // Dojo
{
	evt = {
        hStart: null,
        hEnd: null,
        timer: null,
		msgs: {
			0: "No event",
			1: "Sistema avviato",
			2: "Sistema fermato", 
			4: "Gas acceso",
			8: "Gas spento",
			16: "Pellet acceso",
			32: "Pellet spento",
			64: "Pellet al minimo",
			128: "Pellet in modulazione",
			256: "Pellet FLAMEOUT!",
			512: "Pellet flameout cancellato",
			1024: "Pellet mandata calda",
			2048: "Pellet mandata fredda",
			4096: "Temperatura sopra il target.",
			8192: "Temperatura rientrata sotto il target",
			16384: "Temperatura sotto il target.",
			32768: "Temperatura rientrata sopra il target",
			65536: "Anti-ghiaccio attivato!",
			131072: "Anti-ghiaccio disattivato",
			262144: "Modo manuale impostato",
			524288: "Modo automatico impostato",
			1048576: "Temperatura minima modificata",
			2097152: "Temperatura massima modificata",
			4194304: "Programma modificato",
			8388608: "Sistema attivato",
			16777216: "Sistema disattivato",
            33554432: "Isteresi modificata",
            67108864: "SmartTemp ON",
            134217728: "SmartTemp OFF",
            268435456: "Tempo di spegnimento manuale raggiunto",
            536870912: "Tempo manuale di spegimento modificato",
            1073741824: "Temperatura eccessiva!",
            2147483648: "Temperatura eccessiva rientrata",
            4294967296: "Margine di temperatura eccessiva modificato",
		},	
        toggleRange: function(){
            dclass.toggle(dom.byId("events-range"), "hidden");
            evt.update();
        },
        setData: function(new_rows){
            var len = new_rows.length;
            var pos = 0;
            dc.empty("messages-queue");
            while ( pos < len ){
                var line_end = new_rows.indexOf( "\n", pos );
                if ( line_end != -1 ){
                    var row = new_rows.substr( pos, line_end-pos+1 );
                    var split_row = row.split(" ");                    
                    var t = utils.printDate( split_row[0]*1000 );
                    var m = evt.msgs[ parseInt(split_row[1]) ];
                    var e = m ? m : "-evento-sconosciuto-(" + split_row[1] + ")"; 
                    dc.place("<div>" + t + " -- " + e + "</div>", "messages-queue","first");                    
                    pos = line_end+1;
                } else {
                    if ( new_rows.substr( pos ) == "+" )
                        dc.place("<div>(troppi eventi, solo i primi 120 sono visualizzati)</div>", "messages-queue", "last");
                    pos = len;
                }
            }
        },

		update: function(){
            if ( evt.timer ){
                window.clearTimeout( evt.timer );
                evt.timer = null;
            }

            if ( !eventsUseRange.checked ){
                evt.hStart = new Date();
                evt.hEnd = new Date();
                eventsEnd.set("value", evt.hStart );
                eventsStart.set("value", evt.hEnd );
            } else {
                evt.hEnd = eventsEnd.get("value")
                evt.hStart = eventsStart.get("value")
            }
                var sDate = evt.hStart;sDate.setHours(0);sDate.setMinutes(0);sDate.setSeconds(0);
                var eDate = evt.hEnd;eDate.setHours(23);eDate.setMinutes(59);eDate.setSeconds(59);
				var startDate = Math.floor( sDate / 1000 );
				var endDate = Math.floor( eDate / 1000 ); 

            postRequest("cgi-bin/events",startDate+":"+endDate+":120",
				function(events){
                    evt.setData(events);
                    if ( !eventsUseRange.checked )
                        evt.timer = window.setTimeout( function(){ evt.update(); }, 10*1000 );
				},
				function(err){
					dc.empty("messages-queue");
					dc.place("<li>Impossibile leggere la lista degli eventi!</li>", "messages-queue","first");
                    evt.timer = window.setTimeout( function(){ evt.update(); }, 30*1000 );
				});
		}
	};
    
    on(dom.byId("events-size"),"click", function(v) {
		dclass.toggle(dom.byId("messages"), "history-big");
	});

});
