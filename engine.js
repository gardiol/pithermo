
require([
    "dojo/ready",
    "dojo/domReady!"], 
function(ready)
{		 
	ready(function(){
	   hst.build();
	   sts.build();
		prg.build();
		sts.update();
	   hst.update();
	});
});
