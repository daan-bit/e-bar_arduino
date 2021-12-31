let doorgaan = false;

//Selecteren van drank of frisdrank - veranderd achtergrond
function changeBg(item){

    let list = document.querySelectorAll(".list__listItem");

    for(let i = 0; i < list.length; i++){
        let overlay = list[i].children[0];
        overlay.style.backgroundColor = "#202020";
    }
    
    let child = item.children[0];
    child.style.backgroundColor = "#eee";
    //dorogaan wordt op true gezet, zodat gebruiker naar de volgende pagina kan
    doorgaan = true;


    //Opslaan van soort drank of frisdrank in local storage
    if(item.childNodes[3].textContent.trim() === "Sinas" || item.childNodes[3].textContent.trim() === "Cola"){
        setFris(item);
    }else{
        setDrank(item);
    }
    
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

//Wordt uitgevoerd wanneer random button wordt ingedrukt.
//Kiest willekeurig drank en frisdrank 
function randomMix(){
    let dranken = ["Bacardi", "Malibu", "Vodka", "Rocket"];
    let frissen = ["Cola", "Sinas"];

    let randomDrank = Math.floor(Math.random() * dranken.length);
    let randomFris = Math.floor(Math.random() * frissen.length)

    let drank = dranken[randomDrank];
    let fris = frissen[randomFris];

    //Opslaan van drank en frisdrank in local storage
    localStorage.setItem("drank", drank);
    localStorage.setItem("fris", fris);

    //Versturen van drank en frisdrank naar ESP
    doSend(JSON.stringify({device: "web", drink: drank.substring(0,2), soda: fris.substring(0,2)}));

    window.location = "laden";

}

//Wordt aangeroepen zodra op volgende of klaar wordt geklikt
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
        //Geeft melding dat geen keuze gemaakt is
        const error = document.getElementById("error");
        error.style.transform = "translateY(0rem)";
        setTimeout(closeError, 1500);      
    }

}

//Sluiten van de error
function closeError(){
    const error = document.getElementById("error");
    error.style.transform = "translateY(-15rem)";
}


//Veranderd tekst en kleur in box naar gekozen frisdrank
function changeText(){
    let charFris = localStorage.getItem("fris").substring(0,2);
    let charDrank = localStorage.getItem("drank").substring(0,2);

    let box = document.getElementById("box");

    box.innerText = charDrank + charFris; 

    let soort = localStorage.getItem("fris");
    console.log(soort);
    
    //Veranderd kleur in fill naar soort frisdrank
    if(soort === "Sinas"){
        box.style.setProperty('--fill', '#EB6A0A');
    }else if(soort === "Cola"){
        box.style.setProperty('--fill', '#202020');
    }   
    
}


//##########################################################################
// Alle code behorend bij de websockets ####################################
//##########################################################################

const url = "ws://192.168.4.1:1337/";

window.addEventListener('load', (event)=> {
    wsConnect(url);
});

//Verbinden met server
function wsConnect(url){

    websocket = new WebSocket(url);
    
    websocket.onopen = function(evt) { onOpen(evt) };
    websocket.onclose = function(evt) { onClose(evt) };
    websocket.onmessage = function(evt) { onMessage(evt) };
    websocket.onerror = function(evt) { onError(evt) };
}

//Als client verbind
function onOpen(evt) {
    console.log("Connected");
}

//Als client niet meer verbonden is
function onClose(evt) {
    console.log("Disconnected");
     
    // Proberen opnieuw te verbinden na een aantal seconden
    setTimeout(function() { wsConnect(url) }, 2000);
}

//Als er een bericht binnenkomt
function onMessage(evt) {

    //Print het binnengekomen bericht
    console.log("Received: " + evt.data);
    
    //Switch case voor binnengekomen bericht
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

//Als er een error is
function onError(evt) {
    console.log("ERROR: " + evt.data);
}

//Versturen van bericht naar ESP
function doSend(message) {
    console.log("Sending: " + message);
    websocket.send(message);
}

