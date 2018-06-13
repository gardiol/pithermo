var postRequest = function( req, data, ok_func, ko_func ){
	if ( data.json )
		data = json.stringify(data.json);
	request.post(req, {data:data}).then(ok_func,ko_func);
};	
var putRequest = function( req, data, ok_func, ko_func ){
	request.put(req, {data:data}).then(ok_func,ko_func);
};
var getRequest = function( req, ok_func, ko_func ){
	request(req,{handleAs :"json"}).then(ok_func,ko_func);
};