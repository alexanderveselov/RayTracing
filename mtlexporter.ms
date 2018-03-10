filename = GetSaveFileName types: "OBJ Material (*.MTL) |*.mtl|"
if filename == undefined then
(
	return()
)
outputFile = CreateFile filename

Format "# 3ds Max MTL Exporter by Alexander Veselov\n" to:outputFile
Format "# File Created: %\n\n" localTime to:outputFile

for mtl in sceneMaterials do
(
	if classof mtl != Standardmaterial then continue
	mtlName = substituteString mtl.name " " "_"
	mtlName = substituteString mtlName "-" "_"
	Format "newmtl %\n" mtlName to:outputFile
	Format "type %\n" mtl.shaderType to:outputFile
	Format "diff % % %\n" (mtl.diffuse.r / 255.0) (mtl.diffuse.g / 255.0) (mtl.diffuse.b / 255.0) to:outputFile
	Format "spec % % %\n" (mtl.specular.r / 255.0 * mtl.specularLevel / 100.0) (mtl.specular.g / 255.0 * mtl.specularLevel / 100.0) (
		mtl.specular.b / 255.0 * mtl.specularLevel / 100.0) to:outputFile
	Format "rough %\n" ((100 - mtl.Glossiness) / 100) to:outputFile
	Format "ior %\n" (mtl.ior) to:outputFile
	Format "emit % % %\n" (mtl.selfIllumColor.r / 255.0) (mtl.selfIllumColor.g / 255.0) (mtl.selfIllumColor.b / 255.0) to:outputFile
	Format "\n" to:outputFile	
)

close outputFile