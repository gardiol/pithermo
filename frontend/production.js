var postRequest = null;
var putRequest = null;
var getRequest = null;

require([
    "dojo/request",
    "dojo/json",
    "dojo/domReady!"], 
function( request, json )// Dojo
{
	postRequest = function( req, data, ok_func, ko_func ){
		if ( data.json )
			data = json.stringify(data.json);
		request.post(req, {data:data}).then(ok_func,ko_func);
	};	
	putRequest = function( req, data, ok_func, ko_func ){
		request.put(req, {data:data}).then(ok_func,ko_func);
	};
	getRequest = function( req, ok_func, ko_func ){
		request.get(req, {}).then(ok_func,ko_func);
	};
});
