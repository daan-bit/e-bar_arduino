let doorgaan = false;

//Selecteren van drank of frisdrank
function changeBg(item){

    let list = document.querySelectorAll(".list__listItem");

    for(let i = 0; i < list.length; i++){
        let overlay = list[i].children[0];
        overlay.style.backgroundColor = "#202020";
    }
    
    let child = item.children[0];
    child.style.backgroundColor = "#eee";
    doorgaan = true;


    //Opslaan in van soort drank of frisdrank in local storage
    if(item.childNodes[3].textContent.trim() === "Sinas" || item.childNodes[3].textContent.trim() === "Cola"){
        setFris(item);
    }else{
        setDrank(item);
    }
    
}

//Checkt of er een keuze is gemaakt
function check(pagina){
    
    if(doorgaan === true){
        if(pagina == 'drank'){
            window.location = "kiesFris";
        }else if(pagina == 'shot'){
            localStorage.setItem("fris", "");

            let drink = localStorage.getItem("drank").substring(0,2);
            let soda = localStorage.getItem("fris").substring(0,2);

            doSend(JSON.stringify({device: "shot", drink: drink, soda: soda}));

            window.location = "laden";

             
        }
        else{
            let drink = localStorage.getItem("drank").substring(0,2);
            let soda = localStorage.getItem("fris").substring(0,2);

            doSend(JSON.stringify({device: "web", drink: drink, soda: soda}));

            window.location = "laden";

            
        }
        
    }else{
        const error = document.getElementById("error");
        error.style.transform = "translateY(0rem)";
        setTimeout(closeError, 1500);      
    }

}

function closeError(){
    const error = document.getElementById("error");
    error.style.transform = "translateY(-15rem)";
}

//Gekozen drank opslaan in local storage
function setDrank(item){
    let drank = item.childNodes[3].textContent.trim();
    localStorage.setItem("drank", drank);
}

//Gekozen frisdrank opslaan in local storage
function setFris(item){
    let fris = item.childNodes[3].textContent.trim();
    localStorage.setItem("fris", fris);
}

//Gekozen mix opslaan in local storage
function setMix(drank, fris){
    localStorage.setItem("drank", drank);
    localStorage.setItem("fris", fris);
}

//Veranderd tekst en kleur in box naar gekozen frisdrank
function changeText(){
    let charFris = localStorage.getItem("fris").substring(0,2);
    let charDrank = localStorage.getItem("drank").substring(0,2);

    let box = document.getElementById("box");

    box.innerText = charDrank + charFris; 

    let soort = localStorage.getItem("fris");
    
    //Veranderd kleur in fill naar soort frisdrank
    if(soort === "Sinas"){
        box.style.setProperty('--fill', '#EB6A0A');
    }else if(soort === "Cola"){
        box.style.setProperty('--fill', '#202020');
    }   
    
}


//##########################################################################
//##########################################################################
//##########################################################################

const url = "ws://192.168.4.1:1337/";

window.addEventListener('load', (event)=> {
    wsConnect(url);
});

function wsConnect(url){

    websocket = new WebSocket(url);
    
    websocket.onopen = function(evt) { onOpen(evt) };
    websocket.onclose = function(evt) { onClose(evt) };
    websocket.onmessage = function(evt) { onMessage(evt) };
    websocket.onerror = function(evt) { onError(evt) };
}

function onOpen(evt) {
    console.log("Connected");
}

function onClose(evt) {
    console.log("Disconnected");
     
    // Try to reconnect after a few seconds
    setTimeout(function() { wsConnect(url) }, 2000);
}

function onMessage(evt) {

    // Print out our received message
    console.log("Received: " + evt.data);
    
    // Update circle graphic with LED state
    switch(evt.data) {
        case "0":
            console.log("Naar afgehandeld pagina");
        case "1":
            console.log("Naar foutmelding pagina");
        case "redirect":
            window.location = "/keuzeMenu";
        default:
            break;
    }
}

// Called when a WebSocket error occurs
function onError(evt) {
    console.log("ERROR: " + evt.data);
}

function doSend(message) {
    console.log("Sending: " + message);
    websocket.send(message);
}


//###################################################################
//###################################################################

function raspberry(){
    console.log("Verstuur raspberry pi");
    doSend(JSON.stringify({device: "raspberry", inhoud: "250"}));
}
