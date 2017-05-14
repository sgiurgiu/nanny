var currentStatus = -1;

var displayStatus = function() {
    $.get("/api/block_status",function(result){        
        currentStatus = result.Status;
        if(result.Status === 0) {
            $("#internet_status").html("Internet is closed");
            $("#switch_button").text("Open");
        } else {
            $("#internet_status").html("Internet is open");
            $("#switch_button").text("Close");
        }
        $("#usage").html("Today you have used "+result.Usage+" minutes. Limit is "+result.Limit+" minutes");
    }).fail(function(){
        
    });
}

function changeStatus() {
    if(currentStatus === 0) {
        $.post("/api/allow").always(displayStatus);
    } else {
        $.post("/api/block").always(displayStatus);
    }
}

var refreshFn = function () {
    displayStatus();
    setTimeout(refreshFn,5000);
};

refreshFn();
