let memory; 

function GetWasmStr(pStr)
{
    let ipStr = Number(pStr)
    let length = 0; 

    result = "";
    
    while (memory[ipStr + length] != 0)
    {
        result += String.fromCharCode(memory[ipStr + length]);
        length++;
    }
    
    return result;
}

WebAssembly.instantiateStreaming(
    fetch('/frontend.wasm'),
    {
        env:
        {
            Log: function (s) {
                console.log(GetWasmStr(s));
            },
            Eval: function (s) {
                eval(GetWasmStr(s));
            }
        }
    })
    .then(function(obj) 
    {
        memory = new Uint8Array(obj.instance.exports.memory.buffer);
        console.log(obj.instance.exports.__original_main());
    });