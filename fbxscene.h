#ifndef fbxscene_h
#define fbxscene_h

#include "fbxutil.h"
#include "fbxmath.h"


/*
translator to structured ascii output
*/
struct IWriter {
    //TODO, move to abstract interface.
    FILE* fp;
    IWriter(FILE* _fp) {fp=_fp;seperated=false;}
    ~IWriter() {}
    //todo - indent..
    int x;
    bool seperated;
    void	beginMap() { Seperate(),fprintf(fp, "{\n");seperated=true;}
    void	beginKeyValue(const char* key) {Seperate(); fprintf(fp, "%s:", key); seperated=true;}
    void	endKeyValue() {}
    template<typename T>
    void	keyValue(const char* key, const T& v) { this->beginKeyValue(key); value(v);fprintf(fp,"\n"); endKeyValue();seperated=false;}
    void	endMap() { fprintf(fp, "}\n");seperated=false;}

    void	beginArray(int size) { Seperate();fprintf(fp, "[");seperated=true;}
    void	endArray() { fprintf(fp, "]");seperated=false;}

    void    Seperate() { if (!seperated){fprintf(fp,",");seperated=true;};}
    void	value(int x) { Seperate();fprintf(fp,"%d", x);seperated=false;}
    void	value(float f) { Seperate();fprintf(fp,"%.5f", f);seperated=false;}
    void	value(const char* str) { Seperate();fprintf(fp,"\"%s\" ", str);seperated=false;}

    template<typename T>
    void	keyValueArray(const char* key, const std::vector<T>& src) {
        beginKeyValue(key);
        beginArray(src.size());
        for (auto&n : src) { ::write(this,n); }
        endArray();
        endKeyValue();
    }

    template<typename T>
    void	keyValuePtrArray(const char* key, const std::vector<T>& src) {
        beginKeyValue(key);
        beginArray(src.size());
        for (auto pn : src) { ::write(this,*pn); }
        endArray();
        endKeyValue();
    }

    template<int N>
    void	value(const FbxString<N>& str) { fprintf(fp,"\"%s\" ", str.c_str());}


    template<typename T,int N>
    void	value(const std::array<T,N>& a) {
        beginArray(a.size());
        int	i;
        for (i=0; i<a.size(); i++) value(a[i]);
        endArray();
    }

    template<typename T>
    void	value(const std::vector<T>& a) {
        beginArray(a.size());
        int	i;
        for (i=0; i<a.size(); i++) value(a[i]);
        endArray();
    }

    void	value(const FBXM::Matrix& mat) {
        beginArray(4);
        for (int i=0; i<4; i++) {
            value(mat[i]);
        }
        endArray();
    }
    // todo, why didn't this just work from underlying std::array..
    void	value(const FBXM::Vector4& v) {
        beginArray(4);
        for (int i=0; i<4; i++)
            value(v[i]);
        endArray();
    }
    void	value(const FBXM::Vector3& v) {
        beginArray(3);
        for (int i=0; i<3; i++)
            value(v[i]);
        endArray();
    }
};


class	FbxScene : public FbxMath
{
public:

	static Channel_t	GetChannelIndex(const char* name);
	static const char* getChannelName(int index);
	
	struct	HrcLink {
		HrcLink(int a,int b) {child=a; parent=b;}
		int parent,child;
	};



    typedef Vector3	 Vertex;
    struct	SRT {
        Vector3	scale,rotate,translate;
    };

	enum RotationOrder_t {
		RO_XmYZ
	};
	struct	Triangle : public std::array<int,3>
	{
		Triangle() {};
		Triangle(int i0,int i1, int i2) { auto &t=*this; t[0]=i0; t[1]=i1; t[2]=i2;}
		void	Write(IWriter*) const;
	};
	struct	Quad : public std::array<int,4>
	{
		Quad() {};
		Quad(int i0,int i1, int i2,int i3) {auto &t=*this; t[0]=i0; t[1]=i1; t[2]=i2; t[3]=i3;}
		void	Write(IWriter*) const;
	};
	template<typename OBJECT>
	struct	PropertyDef { const char* name; int dim; float OBJECT::*offset; };

	struct	VertexBoneWeights 
	{
		enum {MAX=4};
		std::array<int,MAX>	boneIndex;
		std::array<float,MAX>	boneWeight;
		int	num;
		int	size() const { return num;}
		int capacity() const { return MAX;}
		void	add(int index, float weight);
		VertexBoneWeights() { num=0; int i; for(i=0;i<MAX;i++) boneWeight[i]=0.f;}
		void	Write(IWriter*) const;
	};

	class	FbxMesh 
	{
	public:
		std::vector<Vertex>	vertices;
		std::vector<VertexBoneWeights>	weightMap;
		std::vector<Triangle>	triangles;
		// todo: quads, as intermediate for strips
		FbxMesh() {}
		void	NormalizeWeightMap();
	};

    class	Model
	{
	public:
        FbxString<128>   name;			// todo: seperate array.
		Vector3	localTranslate,localRotate,localScale;
		float	GetChannel(Channel_t c) const;
		Matrix  localMatrix;
		RotationOrder_t	rotOrder;
		Vector4	weightMapColor;
        Model*			parent;
        std::vector<Model*>    childModels;
        static	PropertyDef<Model>	s_Properties[];
		bool	isDeformer;

		int	meshId;
		Matrix GetLocalMatrix() const;
		bool	HasMesh() const { return meshId>=0;}
        Model();
		void	CalcLocalMatrixFromSRT();
        Matrix	GetLocalMatrixPermuteTest(int permute) const;
	};

	std::string	name;
    std::vector<Model*>	allModels;	// collection owns all models
    std::vector<Model*>  rootModels;	// heirarchy roots
	std::vector<FbxMesh>		meshes;
	std::vector<HrcLink>			hrcLinks;	// parent-child relations, "object-owner"?

    Model*	CreateModel() { auto mdl=new Model(); allModels.push_back(mdl); return mdl;}
    FbxMesh*	CreateMeshForModel(Model* mdl);
    const FbxMesh*	GetMeshOfModel(const Model* mdl) const { return mdl->meshId>=0?&this->meshes[mdl->meshId]:nullptr;}
    FbxMesh*	GetMeshOfModel(Model* mdl) { return &this->meshes[mdl->meshId];}
    Model*  GetModel(const char* mdlName);
	int  GetIndexOfModel(const char* mdlName);
	void	Finalize();
	static const char*	GetRawModelName(const char* src);	// remove namespace qualifier Model::blah

	struct	FCurve
    {
        struct	tvalue { float t, value;
            tvalue(float _t, float _v) {t=_t; value=_v;}
        };
//		short	boneIndex;
//		char	component; char subComponent;
		FbxString<128> boneName; FbxString<128> channelName;
		int	modelIndex; Channel_t channelIndex;
        std::vector<tvalue> points;
	};
	struct	StaticBonePose { 
		short	boneIndex; char component; char subComponent;
		float value;
	};
	class	Take 
	{
	public:
		FbxString<256>	name;
		std::vector<StaticBonePose> staticBones;
		//vector<SRT>		baseSRT;			// TODO:fixed pose precalcualted.
		std::vector<FCurve>	curves;
        void    Setup();
        float   maxt;
	};
	std::vector<Take*>	takes;

    FbxScene() { }
//	~Fbx() { FOR_EACH_IN(m,this->allModels) delete m;}
    ~FbxScene () { for (auto& m :this->allModels) delete m;}

    typedef std::vector<std::array<float, NumChannels> > CycleEvalBuffer;
	void	InitCycleEvalBuffer(CycleEvalBuffer& dst) const;
	void	EvalTake(CycleEvalBuffer& dst, const Take* src, float t) const;	// todo - accumulation
	void	EvalFCurve(CycleEvalBuffer& dst, const FCurve* curve, float t) const;
    void	EvalMatrixArray(Matrix* dst, const CycleEvalBuffer* src) const;
	void	Write(IWriter* dst) const;
    static const char* s_ChannelNames[];
    static Vector4	s_WeightMapColors[4];
    typedef float Model::*ModelFloatPtr_t;
};
void    FbxDumpModel(const FbxScene* scn, const FbxScene::Model* mdl, const FbxMath::Matrix& p);
void	FbxDumpScene(const FbxScene* scn,IWriter* out) ;





// todo,move this somewhere

#endif /* fbxscene_h */