"""Generate original code-drawn Verdant icons and home-menu banner."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[1]
icon=Image.new('RGB',(48,48),'#10241d');d=ImageDraw.Draw(icon)
d.rounded_rectangle((3,3,44,44),radius=9,fill='#294634',outline='#88c572',width=2)
d.polygon([(10,15),(16,15),(24,33),(32,15),(38,15),(27,38),(21,38)],fill='#a7d69b')
icon.save(root/'icon.png')
banner=Image.new('RGB',(256,128),'#10241d');d=ImageDraw.Draw(banner)
for y in range(128):d.line((0,y,255,y),fill=(16+y//12,36+y//8,29+y//10))
banner.paste(icon,(18,40));d.text((76,42),'VERDANT',fill='#a7d69b',font=ImageFont.load_default(size=23));d.text((76,76),'Linux desktop for 3DS',fill='#e2efe6',font=ImageFont.load_default(size=13))
banner.save(root/'banner.png')
large=Image.new('RGB',(128,128),'#10241d');d=ImageDraw.Draw(large)
d.rounded_rectangle((8,8,119,119),radius=22,fill='#294634',outline='#88c572',width=4)
d.polygon([(25,35),(43,35),(64,87),(85,35),(103,35),(73,107),(55,107)],fill='#a7d69b')
# LiveArea requires 8-bit indexed PNGs; RGB PNGs can fail promotion with 0x8010113D.
def save_vita(image,path):
    image.convert('RGB').quantize(colors=256,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE).save(path,bits=8)
save_vita(large,root/'assets/vita-icon0.png')
for name,size in [('bg.png',(840,500)),('startup.png',(280,158))]:
    canvas=Image.new('RGB',size,'#10241d');d=ImageDraw.Draw(canvas)
    for y in range(size[1]):d.line((0,y,size[0],y),fill=(16+y//40,36+y//25,29+y//32))
    d.text((size[0]//8,size[1]//3),'VERDANT',fill='#a7d69b',font=ImageFont.load_default(size=size[0]//14))
    d.text((size[0]//8,size[1]*2//3),'Linux desktop',fill='#e2efe6',font=ImageFont.load_default(size=size[0]//28))
    save_vita(canvas,root/'assets/vita-livearea'/name)
print('Generated original icon.png and banner.png')
