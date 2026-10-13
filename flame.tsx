// flame VN engine
import Ion from "./ion.ts";
const VERSION = {
   major: 0,
   minor: 5,
   patch: 0
}
   
// quark literaly is what react promissed, HTML in javascript/typescript
const modules = import.meta.glob("./scenes/*.tsx", { eager: true });
const scenesData = {}
for (const path in modules) {
   const name = path.split("/").pop().replace(".tsx", "");
   let t = modules[path].default
   if (Array.isArray(t)) {
      // if this is a array, we expect each thing to be like: [<name>, <function>]
      // e.g: [['start', ()=>throw new Error()]]
      t.forEach(def => {
         scenesData[def[0]] = def[1]
      })
   } else
      scenesData[name] = t;
}
  
export type GameState = {
   scene: {
      current: string,
      goto: (n: string) => void
   },
   flags: SStorage,
  
   vars: { [x: string]: any },
};
// @ts-ignore
export let gameState: GameState = {}
console.log(`loaded ${Object.keys(scenesData).length} scene${Object.keys(scenesData).length > 1 ? 's' : ''}`)
//console.log(scenesData)
  
const bgm = new Audio();
bgm.loop = true; // loop music
bgm.volume = 0.5; // nice default volume
document.body.appendChild(bgm); // not *necessary*, but if you want it in DOM for debugging
export function playBGM(src: string) {
   if (bgm.src !== src) {
      bgm.src = src;
      bgm.play();
   }
}
  
// {...children} is the only correct way to display the child nodes
// {children} will not work
/*
❯ node
Welcome to Node.js v23.9.0.
Type ".help" for more information.
> x = [24,52,12]
[ 24, 52, 12 ]
> x.toString()
'24,52,12'
> console.log(...x)
24 52 12
undefined
> console.log(x)
[ 24, 52, 12 ]
undefined
>
*/
export function GoTo({ target, children }) {
   // children is a fucking list([...])
   //console.log(children) // no children nodes? outputs [], otherwise it shows whatever the chil dren nodes are, like ["hello"]
   /*
     <GoTo target={"help"} >help</GoTo>
 
     makes this log ["help"] for example
   */
   const exists = target in scenesData
  
   if (exists)
      return <a href="#" onClick={e => { e.preventDefault(); gameState.scene.goto(target) }}>
         {...children} {/*typeof children == "object"*/}
      </a>
   else // style={{ color: "red", cursor: "not-allowed" }} is not handled in quark, since that is  not how HTML works
      return <a href="#" onClick={e => e.preventDefault()} style="color:red; cursor:not-allowed;" title={`scene "${target}" not found`}>{...children}</a>
}
  
export function Input(varName, target, placeholder = "") {
   return <input placeholder="name" onInput={(e) => gameState.vars.playerName = e.target.value} o nChange={() => gameState.scene.goto("intro")} />
}
  
export function scene(name: string, fn: (state: GameState) => HTMLElement) {
   return {
      name,
      call: fn
   }
}
export class SStorage {
   private data: { [id: string]: any } = {}
   has(t: string): boolean {
      return t in this.data;
   }

   keys() {
      return Object.keys(this.data)
   }

   get(t: string): any {
      return this.data[t]
   }

   remove(t: string) {
      delete this.data[t]
   }

   set(t: string, v: any) {
      this.data[t] = v
   }
}

/*
const scenes: { [name: string]: (state: GameState) => HTMLElement } = {
  mainMenu: (state) => {
    state.vars.money = 0
    return <div class="scene">
      <h1>Main Menu</h1>
      <button onClick={() => state.scene.goto("game")}>Start Game</button>

    </div>
  },

  game: (state) => (
    <div class="scene">
      <h1>Game Scene</h1>
      <button onClick={() => state.scene.goto("mainMenu")}>Back to Menu</button>
      <button onClick={() => {
        state.vars.money++
        // sugarcube doesn't reload/rerun scenes if you don't use <<goto name>> :)
        state.scene.goto(state.scene.current)
      }}>add money</button>
    </div>
  )
};
*/

const flags = new SStorage()
const vars = {}

// this is basicly what monogatari and twine/tweego/sugarcube does, but with less effects
function App() {
   const [scene, goto] = Ion.useState("start")

   gameState = {
      scene: {
         current: scene,
         goto
      },
      flags,
      vars,
   };
   //console.log(scenesData[scene], scene, scenesData)
   return (
      <div class="app">
         <div class="scene">
            {scenesData[scene] ? scenesData[scene](gameState) : <div>
               <div>
                  Scene not found: {scene}
               </div>
               <GoTo target={"start"}>go back to the menu</GoTo>
            </div>}
         </div>

      </div>
   );
}

const root = document.getElementById("root")!;

Ion.render(App, root);
console.log(`flame ${JSON.stringify(VERSION)} - set fire to shit ideas`) // if you see this, it means the engine + the scenes actualy loaded correctly
