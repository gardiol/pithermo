
var utils = {
    printDate:function(d){return utils.date2str(d)+" "+utils.time2str(d);},
    date2str:function(d){
        d =d instanceof Date?d:new Date(d);
        var dy=d.getDate();
        var dm=d.getMonth()+1;
        return(dy<10?"0":"")+dy+"/"+(dm<10?"0":"")+dm+"/"+(""+d.getFullYear()).substr(2);                         
    },
    time2str:function(d){
        d =d instanceof Date?d:new Date(d);
        var dh=d.getHours();
        var dM=d.getMinutes();
        return(dh<10?"0":"")+dh+":"+(dM<10?"0":"")+dM;                            
    }

    
};
