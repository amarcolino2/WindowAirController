
var connection = new WebSocket('ws://'+location.hostname+':81/', ['arduino']);

connection.onopen = function () {
  connection.send('Connect ' + new Date());
};
connection.onerror = function (error) {
  console.log('WebSocket Error ', error);
  alert('WebSocket Error ', error);
};
connection.onmessage = function (e) {  
  message(e);
};
connection.onclose = function(){
  console.log('WebSocket connection closed');
  alert('WebSocket connection closed');
};

function getOnLoad() { 
  connection.send('Connect ' + new Date());
};


function setBroker() {
  window.location.href = "setup.html";
}
function setWifi() {
  window.location.href = "setWifi.html";
}
function home() {
  window.location.href = "index.html";
}
function powerText() {
  window.location.href = "powerTest.html";
}

function message(e){
  let Code = e.data;
  console.log(Code);
  
  if(Code=='autotication:OK'){
    window.location.href = "setup.html";
    return;
  }
  else if(Code=='autotication:Incorrect'){
    alert('Incorrect');
    return;
  }
  if(Code=='setNetWork:OK'){
    alert('Saved');
      return;
  }
  else{
    alert('Error');
  }
  if(Code=='setWifi:OK'){
    alert("Saved");
      return;
  }
  /*if(Code.substring(0,10)=='Power: Off'){
      recCode = Code;
      rainbowEnable = false;
      document.getElementById("power").innerHTML = "Off";
      document.getElementById("irCodeRead").innerHTML = recCode;
      return;
  }
  if(Code.substring(0,5)=='Temp:'){
      recCode = Code;
      document.getElementById("irCodeRead").innerHTML = recCode;
      return;
  }
  if(Code.substring(0,12)=='low air flow'){
      recCode = Code;
      document.getElementById("irCodeRead").innerHTML = recCode;
      return;
  }
  document.getElementById("irCodeCopy").innerHTML = Code;*/
};

//$('.teste').click(() => window.location = 'pagina.html');

$(document).ready(function() {
    const lockModal = $("#lock-modal");
    const loadingCircle = $("#loading-circle");
    const form = $("#my-form");
  
    form.on('submit', function(e) {
      e.preventDefault(); //prevent form from submitting
  
      const firstname = $("input[name=user_login]").val();
      const lastname = $("input[name=user_pass]").val();
  
      // lock down the form
      lockModal.css("display", "block");
      loadingCircle.css("display", "block");
  
      form.children("input").each(function() {
        $(this).attr("readonly", true);
      });
    
      setTimeout(function() {
        // re-enable the form
        lockModal.css("display", "none");
        loadingCircle.css("display", "none");
  
        form.children("input").each(function() {
          $(this).attr("readonly", false);
        });
  
        form.append(`<p>Thanks ${firstname} ${lastname}!</p>`);
      }, 3000);
    });  
 });

 function autentication(){
  let user = document.getElementById("user").value;
  let pass = document.getElementById("pass").value;
  
  if(user!="" && pass != ""){
    console.log("user:"+user +","+ "pass:"+pass);
    connection.send("user:"+user +","+ "pass:"+pass);
  }
  else{
    alert('Nao pode conter campo vazio')
  }
  

};

function mqtt(){
  let mqttIp = document.getElementById("brokerIp").value;
  let mqttPort = document.getElementById("brokerPort").value;
  let brokerUser = document.getElementById("brokerUser").value;
  let brokerpass = document.getElementById("brokerPass").value;
  if(mqttIp!="" && mqttPort != "" && brokerUser != "" && brokerpass != ""){
    console.log("ip:"+mqttIp +","+ "port:"+mqttPort+"user:"+brokerUser+"pass:"+brokerpass);
    connection.send("ip:"+mqttIp +","+ "port:"+mqttPort+"user:"+brokerUser+"pass:"+brokerpass);
  }
  else{
    alert('Nao pode conter campo vazio')
  }
};

function wifi(){
  let ssid = document.getElementById("Wssid").value;
  let pass = document.getElementById("Wpass").value;
  
  if(ssid!="" && pass != ""){
    console.log("ssid:"+ssid +","+ "pass:"+pass);
    connection.send("ssid:"+ssid +","+ "pass:"+pass);
  }
  else{
    alert('Nao pode conter campo vazio');
  }
};

function power() {
  let power = document.getElementById("powerButton").value;
  if(power == "off"){
    document.getElementById("powerButton").value = "on";
    document.getElementById('powerButton').innerHTML = "On ";
    
  }
  else{
    document.getElementById("powerButton").value = "off";
    document.getElementById('powerButton').innerHTML  = "Off";
    
  }
  
}