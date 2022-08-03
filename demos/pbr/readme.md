## Detail

- A very simply PBR ball, may contain some problems.

- D using GGX, F using Schlick approximation (Hard Code), G using SchlicksmithGGX.

- Metallic growing up.
![](m15_r15.png)
![](m60_r15.png)
![](m100_r15.png)

- Roughness growing up.
![](m60_r15.png)
![](m60_r50.png)
![](m60_r75.png)



## Trap

- This time should handle UI and data binding, each change on data response for a memcpy is bad.


## Further

- Energy lose , using `Kulla-Conty Approximate`  to fix.