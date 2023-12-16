#version 150

uniform int u_Time;

in vec2 fs_UV;

out vec4 out_Col;

uniform sampler2D u_RenderedTexture;
void main()
{
    float TAU = 6.28318530718;
    int MAX_ITER = 5;

    float time = u_Time * 0.5+23;
    time/=100.0;
    vec2 p = mod(fs_UV*TAU, TAU)-250.0;


    vec2 i = vec2(p);
    float c = 1.0;
    float inten = .005;

    for (int n = 0; n < MAX_ITER; n++)
    {
            float t = time * (1.0 - (3.5 / float(n+1)));
            i = p + vec2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));
            c += 1.0/length(vec2(p.x / (sin(i.x+t)/inten),p.y / (cos(i.y+t)/inten)));
    }

    c /= float(MAX_ITER);
    c = 1.17-pow(c, 1.4);
    vec3 colour = vec3(pow(abs(c), 8.0));
    colour = 1-colour;
    colour.r *=2;

    out_Col = clamp(vec4(colour,1)*0.25+vec4(texture(u_RenderedTexture, fs_UV).r,0,0,texture(u_RenderedTexture, fs_UV).a),0,1);
}
