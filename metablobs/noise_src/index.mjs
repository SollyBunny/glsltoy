import { createNoise4D } from "./noise.mjs";

const can = document.getElementById("can");
const ctx = can.getContext("2d");

const noise4D = createNoise4D();

/*

How this works

1. Sample a 3D sphere from the noise function
2. Sample the 3D sphere's surface to get a 2D noise
3. This should loop in x and y if done right

*/

function sphericalToCartesian(phi, theta, r) {
	const x = r * Math.sin(phi) * Math.cos(theta);
	const y = r * Math.sin(phi) * Math.sin(theta);
	const z = r * Math.cos(phi);
	return { x, y, z };
}
function sphericalToCube(phi, theta, r) {
	const x = r * Math.sin(phi) * Math.cos(theta);
	const y = r * Math.sin(phi) * Math.sin(theta);
	const z = r * Math.cos(phi);
  
	// Convert spherical coordinates to the Lambert azimuthal equal-area projection
	const lambda = theta;
	const phiPrime = Math.PI / 2 - phi;
  
	const cubeX = x;
	const cubeY = y;
	const cubeZ = z;
  
	return { x: cubeX, y: cubeY, z: cubeZ };
}

const size = 512;
const r = size * 2e-4;
can.width = size;
can.height = size;
const imageData = new ImageData(size, size);
function noisetonot(x) {
	return Math.floor((x + 1) * 128)
}
function draw() {
	ctx.clearRect(0, 0, size, size);
	for (let a = 0; a < size; ++a) {
		for (let b = 0; b < size; ++b) {
			const {x, y, z} = sphericalToCube(
				a / size * 2 * Math.PI,
				b / size * 2 * Math.PI,
				r
			);
			const i = (a * size + b) * 4; // 4 channels: Red, Green, Blue, Alpha
			imageData.data[i] = noisetonot(noise4D(x, y, z, 1e4));
			imageData.data[i + 1] = noisetonot(noise4D(x, y, z, 2e4));
			imageData.data[i + 2] = noisetonot(noise4D(x, y, z, 3e4));
			imageData.data[i + 3] = noisetonot(noise4D(x, y, z, 4e4));
		}
	}
	ctx.putImageData(imageData, 0, 0);
}
draw();
window.imageData = imageData;
window.draw = draw;
window.noise4D = noise4D;
window.sphericalToCartesian = sphericalToCartesian;
console.log("Done!")
