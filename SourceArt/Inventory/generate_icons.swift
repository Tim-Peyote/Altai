import AppKit
// Original simple inventory symbols; regenerate PNGs with swift generate_icons.swift <output-dir>.
let folder = CommandLine.arguments[1]
func color(_ r:CGFloat,_ g:CGFloat,_ b:CGFloat,_ a:CGFloat=1)->NSColor { NSColor(calibratedRed:r,green:g,blue:b,alpha:a) }
let bone=color(0.77,0.75,0.64), shadow=color(0.27,0.31,0.29), accent=color(0.59,0.48,0.30)
func polygon(_ points:[CGPoint],_ fill:NSColor,_ stroke:NSColor=bone) {
 let p=NSBezierPath();p.move(to:points[0]);for x in points.dropFirst(){p.line(to:x)};p.close();fill.setFill();p.fill();stroke.setStroke();p.lineWidth=3;p.stroke()
}
func line(_ points:[CGPoint],_ c:NSColor=bone,_ width:CGFloat=4){let p=NSBezierPath();p.move(to:points[0]);for x in points.dropFirst(){p.line(to:x)};c.setStroke();p.lineWidth=width;p.lineCapStyle = .round;p.stroke()}
func pt(_ x:CGFloat,_ y:CGFloat)->CGPoint{CGPoint(x:x,y:y)}
for name in ["Stone","Fiber","Flask"] {
 let bitmap=NSBitmapImageRep(bitmapDataPlanes:nil,pixelsWide:256,pixelsHigh:256,bitsPerSample:8,samplesPerPixel:4,hasAlpha:true,isPlanar:false,colorSpaceName:.deviceRGB,bytesPerRow:0,bitsPerPixel:0)!
 NSGraphicsContext.saveGraphicsState();NSGraphicsContext.current=NSGraphicsContext(bitmapImageRep:bitmap)
 NSColor.clear.setFill();NSRect(x:0,y:0,width:256,height:256).fill(using:.copy)
 if name=="Stone" {
  polygon([pt(42,81),pt(57,152),pt(118,192),pt(188,163),pt(217,101),pt(169,60),pt(95,56)],shadow)
  polygon([pt(57,152),pt(118,192),pt(147,131),pt(98,92),pt(42,81)],color(0.43,0.45,0.40))
  line([pt(147,131),pt(188,163)]);line([pt(147,131),pt(169,60)]);line([pt(98,92),pt(95,56)],accent)
 } else if name=="Fiber" {
  for i in 0..<5 {
   let x=CGFloat(83+i*20), top=CGFloat(190+(i%2)*18)
   line([pt(117,42),pt(x,116),pt(x-9,top)],accent,3)
   polygon([pt(x-3,154),pt(x-31,173),pt(x-27,193),pt(x-7,175)],color(0.32,0.39,0.27),accent)
   polygon([pt(x-6,130),pt(x+18,151),pt(x+23,178),pt(x+1,160)],color(0.40,0.47,0.32),bone)
  }
  line([pt(102,65),pt(138,72)],bone,7)
 } else {
  polygon([pt(102,191),pt(102,166),pt(69,142),pt(65,67),pt(85,47),pt(171,47),pt(191,67),pt(187,142),pt(154,166),pt(154,191)],shadow)
  polygon([pt(101,189),pt(101,211),pt(155,211),pt(155,189)],accent)
  line([pt(79,130),pt(177,130)],accent,8);line([pt(81,77),pt(81,116)],bone,5)
  line([pt(100,60),pt(159,60)],accent,3)
 }
 NSGraphicsContext.restoreGraphicsState()
 try bitmap.representation(using:.png,properties:[:])!.write(to:URL(fileURLWithPath:folder+"/T_"+name+".png"))
}
