 var evt = null;
 
 require([
    "dojo/dom", 
    "dojo/dom-construct",
    "dojo/domReady!"], 
function( dom, dc)    // Dojo
{
	evt = {
		events: [],
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
			4096: "Sovra-temperatura rilevata!",
			8192: "Temperatura rientrata sotto il massimo",
			16384: "Temperatura sotto il minimo rilevata!",
			32768: "Temperatura rientrata sopra il minimo",
			65536: "Anti-ghiaccio attivato!",
			131072: "Anti-ghiaccio disattivato",
			262144: "Modo manuale impostato",
			524288: "Modo automatico impostato",
			1048576: "Temperatura minima modificata",
			2097152: "Temperatura massima modificata",
			4194304: "Programma modificato",
			8388608: "Sistema attivato",
			16777216: "Sistema disattivato",
		},	
		update: function(){
			getRequest("cgi-bin/events",
				function(events){
					var s = evt.events.length;
					var e = events.length;
					if ( (evt.events == []) || 
						(events.length < evt.events.length) ){
						dc.empty("messages-queue");
						s = 0;
					}	
					evt.events = events;                 
					for ( var x = s; x < evt.events.length; x++ ){
						var se = evt.events[x];
				      var t = new Date(se.t*1000).toLocaleString();
				      var e = evt.msgs[se.e] ? evt.msgs[se.e] : evt.msgs[0] + "(" + se.e + ")";                
						dc.place("<li>" + t + " -- " + e + "</li>", "messages-queue","first");
					}
				},
				function(err){
					dc.empty("messages-queue");
					dc.place("<li>Impossibile leggere la lista degli eventi!</li>", "messages-queue","first");
				});
		}, 
 		disable: function(){
 					evt.events = [];                
					dc.empty("messages-queue");
					dc.place("<li>Errore di connessione</li>", "messages-queue","first");
		}
	};
}