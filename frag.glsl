#version 330
out vec4 FragColor;

uniform vec3 lightPos;
uniform vec3 eyePos;

vec3 I = vec3(0.85);
vec3 Iamb = vec3(0.8, 0.8, 0.8);

uniform vec3 color;
vec3 ka = vec3(0.1, 0.1, 0.1);
vec3 ks = vec3(0.8, 0.8, 0.8);

in vec4 fragPos;
in vec3 N;

uniform bool isCheckboard = false;
uniform float scale = 0.1f;
uniform float offset = 10.0f;

void main(void)
{
	if(isCheckboard)
	{
		vec3 pos = vec3(fragPos);
		float xf = floor((pos.x) * scale);
		xf = xf - (2 * floor(xf/2));
		bool x = xf > 0;

		float yf = floor((pos.y) * scale);
		yf = yf - (2 * floor(yf/2));
		bool y = yf > 0;

		float zf = floor((pos.z + offset) * scale);
		zf = zf - (2 * floor(zf/2));
		bool z = zf > 0;
		
		bool xorXY = x != y;

		if (xorXY != z) {
			FragColor = vec4(0.0, 0.0, 50.0, 255.0)/255.0; // Return black
		} else {
			FragColor = vec4(75.0, 127.0, 229.0, 255.0)/255.0; // Return white
		}
		return;
	}

	vec3 L = normalize(lightPos - vec3(fragPos));
	vec3 V = normalize(eyePos - vec3(fragPos));
	vec3 H = normalize(L + V);
	float NdotL = dot(N, L);
	float NdotH = dot(N, H);

	vec3 diffuseColor = I * color * max(0, NdotL);
	vec3 ambientColor = Iamb * ka;
	vec3 specularColor = I * ks * pow(max(0, NdotH), 20);

    FragColor = vec4(diffuseColor + ambientColor + specularColor, 1);
}
