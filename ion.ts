// ion.ts - quark.ts but extended to have the
// global thing(quark is just the tsx stuff bellow)
declare global {
	interface HTMLElement {
		$style(obj: Partial<CSSStyleDeclaration>): this;
		$click(handler: (e: Event) => void): this;
		$on(event: string, handler: (e: Event) => void): this;
		$attr(name: string): string | null;
		$attr(name: string, value: string | null | undefined): this;
	}
	interface HTMLImageElement {
		$resize(width: number, height: number): this
	}
	interface HTMLCanvasElement {
		$resize(width: number, height: number): this
		$copy(other: HTMLCanvasElement): this
	}

	interface CanvasRenderingContext2D {
		$rect(x: number, y: number, width: number, height: number, color: string): this
		$clear(color: string): this
		$text(text: string, x: number, y: number, color: string, font: string): this
		$beginCamera(transform: {
			x: number
			y: number
			scaleX: number
			scaleY: number
			rotation: number
			offsetX: number
			offsetY: number
		}): this
		$endCamera(): this
	}

	interface Window {
		$on(event: string, handler: (e: Event) => void): void
	}

	namespace JSX {
		type Element = HTMLElement;
		type IntrinsicElements = {
			[K in keyof HTMLElementTagNameMap]: Partial<HTMLElementTagNameMap[K]>; // fixed :D
		}
	}
}

window.$on = (event: string, handler: (e: Event) => void) => {
	window.addEventListener(event, handler);
}

HTMLElement.prototype.$on = function (event: string, handler: (e: Event) => void) {
	this.addEventListener(event, handler);
	return this;
}

HTMLImageElement.prototype.$resize = function (width: number, height: number) {
	this.width = width
	this.height = height
	return this
}

HTMLElement.prototype.$click = function (handler) {
	// should we make this be "onclick" instead?
	this.addEventListener("click", handler);
	return this;
}

HTMLElement.prototype.$attr = function (name: string, value?: string | null) {
	if (value === undefined) return this.getAttribute(name);
	if (value == null) this.removeAttribute(name);
	else this.setAttribute(name, value);
	return this;
};

HTMLElement.prototype.$style = function (obj: Partial<CSSStyleDeclaration>) {
	Object.assign(this.style, obj);
	return this;
};

HTMLCanvasElement.prototype.$resize = function (width, height) {
	this.width = width
	this.height = height
	return this
}

HTMLCanvasElement.prototype.$copy = function (other) {
	this.$resize(other.width, other.height)
	return this
}
CanvasRenderingContext2D.prototype.$rect = function (x, y, width, height, color) {
	this.fillStyle = color
	this.fillRect(x, y, width, height)
	return this
}

CanvasRenderingContext2D.prototype.$clear = function (color) {
	this.fillStyle = color
	this.fillRect(0, 0, this.canvas.width, this.canvas.height)
	return this
}

CanvasRenderingContext2D.prototype.$text = function (text, x, y, color, font) {
	this.fillStyle = color
	this.font = font
	this.fillText(text, x, y)
	return this
}

CanvasRenderingContext2D.prototype.$beginCamera = function (transform: {
	x: number
	y: number
	scaleX: number
	scaleY: number
	rotation: number
	offsetX: number
	offsetY: number
}) {
	this.save()
	this.translate(transform.offsetX, transform.offsetY)
	this.rotate(transform.rotation)
	this.scale(transform.scaleX, transform.scaleY)
	this.translate(-transform.x, -transform.y)
	return this
}

CanvasRenderingContext2D.prototype.$endCamera = function () {
	this.restore()
	return this
}

let states: any[] = []; // Global state storage
let setStateCallback: (() => void) | null = null; // Callback for re-rendering
let stateId = 0; // Used to track state slots
// --- helpers: flatten children robustly ---
function flattenChildren(children: any[]): any[] {
	const out: any[] = [];

	const add = (c: any) => {
		if (c === null || c === undefined) return;            // skip null/undefined
		if (typeof c === "boolean") return;                   // skip booleans like React
		if (Array.isArray(c)) { c.forEach(add); return; }     // recurse arrays
		// handle NodeList / HTMLCollection
		if (typeof NodeList !== "undefined" && c instanceof NodeList) {
			Array.from(c).forEach(add);
			return;
		}
		if (typeof HTMLCollection !== "undefined" && c instanceof HTMLCollection) {
			Array.from(c).forEach(add);
			return;
		}
		out.push(c);
	};

	children.forEach(add);
	return out;
}

// --- use it inside jsx ---
export function jsx(type: any, props: any, ...children: any[]): HTMLElement {
	if (!props) props = {};

	if (typeof type === "function") {
		props.children = children;
		return type(props);
	}

	const domElem = type === "Fragment"
		? document.createDocumentFragment()
		: document.createElement(type);

	// properties
	for (const prop in props) {
		if (prop === "children") continue;

		if (prop.startsWith("on")) {
			const eventName = prop.substring(2).toLowerCase();
			domElem.addEventListener(eventName, (props as any)[prop]);
		} else {
			// guard for DocumentFragment (no setAttribute)
			if ((domElem as any).setAttribute) (domElem as any).setAttribute(prop, (props as any)[prop]);
			else (domElem as any)[prop] = (props as any)[prop];
		}
	}

	// append flattened children
	const flat = flattenChildren(children);
	for (const child of flat) {
		if (typeof child === "string" || typeof child === "number") {
			domElem.appendChild(document.createTextNode(String(child)));
		} else if (child instanceof Node) {
			domElem.appendChild(child);
		} else {
			// fallback: stringify anything else (Date, custom objects with toString, etc.)
			domElem.appendChild(document.createTextNode(String(child)));
		}
	}

	return domElem;
}


export const Fragment = "Fragment";

// Improved useState with re-rendering support

function useState<T>(initialState?: T) {
	const vId = stateId++; // Unique ID for each state

	// Initialize state if it hasn't been set yet
	if (typeof states[vId] === "undefined") {
		states[vId] = initialState;
	}

	const setState = (newState: any) => {
		states[vId] = typeof newState === "function" ? newState(states[vId]) : newState;

		if (setStateCallback) setStateCallback(); // Trigger re-render
	};

	return [states[vId], setState];
}

// Function to trigger re-rendering
function render(Component: any, root: HTMLElement) {
	setStateCallback = () => {
		stateId = 0; // Reset ID for the new render
		root.innerHTML = ""; // Clear the current DOM
		root.appendChild(Component()); // Re-render with the new state
	};

	root.appendChild(Component()); // Initial render
}

// Export as default object
export default {
	createElement: jsx,
	Fragment,
	useState,
	render
};