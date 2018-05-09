
uniform sampler2D diffuse;
uniform float alpha;
varying vec2 texCoordVar;

void main() {
    gl_FragColor = texture2D(diffuse, texCoordVar) * vec4(1.0, 1.0, 1.0, alpha);
}

