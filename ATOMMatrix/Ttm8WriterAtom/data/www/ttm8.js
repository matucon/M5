var timeoutMs = 10000;

var runningModal = new bootstrap.Modal(document.getElementById('runningModal'), {
  keyboard: false
});

function sendCode() {
  runningModal.toggle();

	$.ajax({
		url:"/write",
		type:"POST",
		data:{runcode: $('#runcode').val(), cleansing: $('#cleansing').is(":checked")}, 
		dataType:"json",
    timeout: timeoutMs,
  }).done(function(data1,textStatus,jqXHR) {

  }).fail(function(jqXHR, textStatus, errorThrown ) {
    alert('失敗しました。');
  }).always(function(){
    runningModal.toggle();
  });
}

function clearCode() {
  $("#runcode").val("");
}

function sendRamClear() {
  runningModal.toggle();

	$.ajax({
		url:"/ramclear",
		type:"GET",
    data: {},
		dataType:"json",
    timeout: timeoutMs,
  }).done(function(data1,textStatus,jqXHR) {

  }).fail(function(jqXHR, textStatus, errorThrown ) {
    alert('失敗しました。');
  }).always(function(){
    runningModal.toggle();
  });
}

function sendRamClearRange() {
  runningModal.toggle();

	$.ajax({
		url:"/ramclear",
		type:"GET",
    data: {from: $('#addFrom').val(), to: $('#addTo').val()},
		dataType:"json",
    timeout: timeoutMs,
  }).done(function(data1,textStatus,jqXHR) {

  }).fail(function(jqXHR, textStatus, errorThrown ) {
    alert('失敗しました。');
  }).always(function(){
    runningModal.toggle();
  });
}

function sendAddr(shift) {
  $.ajax('/regista/address',
    {
      type: 'get',
      data: {shift: shift},
      dataType: 'entry'
    }
  );
}

function sendAddrValue(value) {
  var json = (value == null) ? {value: $('#valueAddr').val()} : {value: value};
  $.ajax('/regista/address',
    {
      type: 'get',
      data: json,
      dataType: 'entry'
    }
  );
}

function sendData(shift) {
  $.ajax('/regista/data',
    {
      type: 'get',
      data: {shift: shift},
      dataType: 'entry'
    }
  );
}

function sendDataValue(value) {
  var json = (value == null) ? {value: $('#valueData').val()} : {value: value};
  $.ajax('/regista/data',
    {
      type: 'get',
      data: json,
      dataType: 'entry'
    }
  );
}

function sendRclk() {
  $.ajax('/rclk', {type: 'get',dataType: 'entry'});
}

function sendRamwe() {
  $.ajax('/ramwe', {type: 'get',dataType: 'entry'});
}

function sendReset() {
  $.ajax('/reset', {type: 'get',dataType: 'entry'});
}

function sendGpio(no, flag) {
  $.ajax('/gpio',
    {
      type: 'get',
      data: { no: no, flag: flag },
      dataType: 'entry'
    }
  );
}

$(function() {
  // 画面にM5ATOMのIPを表示
	$.ajax({
		url:"/ip",
		type:"GET",
    timeout: timeoutMs,
  }).done(function(data1,textStatus,jqXHR) {
    var data_json = JSON.parse(JSON.stringify(data1));
    $("#ipaddr").text(data_json.ip);
  });

  $('#statusGpio').on('change', function() {
    $.ajax({
      url:"/gpio/status",
      type:"GET",
      data:{status: $('#statusGpio').is(":checked")}, 
      dataType:"json",
      timeout: timeoutMs,
    }).done(function(data1,textStatus,jqXHR) {
  
    }).fail(function(jqXHR, textStatus, errorThrown ) {
      alert('失敗しました。');
    }).always(function(){

    });
  });
});
