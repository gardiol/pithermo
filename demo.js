var postRequest = null;
var putRequest = null;
var getRequest = null;

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
 "dojo/domReady!"], 
function( request, dom, attr, dclass, style, domConstruct, html, query, json, on, win )
{
	var local_status = {"mode":"manual","active":"on","antiice":"off",
					    "warnings":{"modeswitch":""},
						"pellet":{"command":"off","status":"off","minimum":"on","flameout":"off","time":"0","mintime":"0"},
						"gas":{"command":"off","time":"0","status":"off"},
						"temp":{"min":17.3,"max":20.2,"int":19.7,"ext":22.7,"hum":61.6,"ext_hum":74.5},
						"now":{"d":2,"h":16,"f":0},
						"program":[["o","o","o","o","o","o","o","o","o","o","o","o","o","g","g","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","g","g","o","o","o","o","o","o","o"],["o","o","o","o","o","o","o","o","o","o","o","o","o","g","g","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o"],["o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","g","g","o","o","g","o","o","o","o","o"],["o","o","o","o","o","o","o","o","o","o","o","o","o","g","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","g","o","o","g","o","o","g","o","o","g","o","o","o","o","o","o","o","o"],["o","o","o","o","o","o","o","o","o","o","o","o","o","g","g","g","g","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","g","o","g","o","g","o","g","o","o","o","o","o","o","o","o","o"],["o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","g","g","o","o","o","o","o"],["o","o","o","o","o","o","o","o","o","o","o","o","o","o","g","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o","o"]]};

	
	var local_history = "1000 20 80 5 10\n2000 21 75 4 20\n3000 22 70 3 30\n4000 23 65 2 35";
	var local_events = [{"t":1525582853,"e":4},{"t":1525584653,"e":8},{"t":1525600602,"e":16777216}];
	
	putRequest = function( req, data, ok_func, ko_func ){
		else if ( data == "activate" ){
			local_status.active = "on";
			ok_func();
		}
		else if ( data == "deactivate" ){
			local_status.active = "off";
			ok_func();
		}
		else if ( data == "manual" ){
			local_status.mode = "manual";
			ok_func();
		}
		else if ( data == "auto" ){
			local_status.mode = "auto";
			ok_func();
		}
		else if ( data == "pellet-on" ){
			local_status.pellet.command = "on";
			local_status.pellet.status = "on";
			ok_func();
		}
		else if ( data == "pellet-off" ){
			local_status.pellet.command = "off";
			local_status.pellet.status = "off";
			ok_func();
		}
		else if ( data == "pellet-minimum-on" ){
			local_status.pellet.minimum = "on";
			ok_func();
		}
		else if ( data == "pellet-minimum-off" ){
			local_status.pellet.minimum = "off";
			ok_func();
		}
		else if ( data == "gas-on" ){
			local_status.gas.command = "on";
			local_status.gas.status = "on";
			ok_func();
		}
		else if ( data == "gas-off" ){
			local_status.gas.command = "off";
			local_status.gas.status = "off";
			ok_func();
		}
		else{
			alert( data );
			ko_func();
		}
	}

	postRequest = function( req, data, ok_func, ko_func ){
		if ( req == "cgi-bin/set_min_temp" ){
			local_status.temp.min = data.data;
			ok_func();
		}	
		else if ( req == "cgi-bin/set_max_temp" ){
			local_status.temp.max = data.data;
			ok_func();
		}	
		else if ( req == "cgi-bin/program" ){
			local_status.program = data.json;
			ok_func();
		}	
		else if ( req == "cgi-bin/history" ){
			console.log( data );
			ok_func( local_history );
		}	
	}

	getRequest = function( req, ok_func, ko_func ){
		if ( req == "cgi-bin/status" ){
			var dat = new Date();
			var dy = dat.getDay()-1;
			if ( dy < 0 ) dy = 6;
			local_status.now = { d: dy, h: dat.getHours(), f: dat.getMinutes() > 30 ? 1 : 0 };
			ok_func( local_status );
		}	
		else if ( req == "cgi-bin/events" ){
			ok_func( local_events );
		}	
	}
});
