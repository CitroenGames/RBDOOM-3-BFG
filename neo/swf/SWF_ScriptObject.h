#pragma once

class idSWFSpriteInstance;

/*
========================
This is the base class for script variables which are implemented in code
========================
*/
class idSWFScriptNativeVariable
{
public:
	virtual bool IsReadOnly()
	{
		return false;
	}
	virtual void Set( class idSWFScriptObject* object, const idSWFScriptVar& value ) = 0;
	virtual idSWFScriptVar Get( class idSWFScriptObject* object ) = 0;
};

#define SWF_NATIVE_VAR_DECLARE( x ) \
	class idSWFScriptNativeVar_##x : public idSWFScriptNativeVariable {			\
	public:																		\
		void Set( class idSWFScriptObject * object, const idSWFScriptVar & value );	\
		idSWFScriptVar Get( class idSWFScriptObject * object );					\
	} swfScriptVar_##x;

#define SWF_NATIVE_VAR_DECLARE_READONLY( x )									\
	class idSWFScriptNativeVar_##x : public idSWFScriptNativeVariable {			\
	public:																		\
		bool IsReadOnly() { return true; }										\
		void Set( class idSWFScriptObject * object, const idSWFScriptVar & value ) { assert( false ); } \
		idSWFScriptVar Get( class idSWFScriptObject * object );					\
	} swfScriptVar_##x;

/*
========================
This is a helper class for quickly setting up native variables which need access to a parent class
========================
*/
template< typename T >
class idSWFScriptNativeVariable_Nested : public idSWFScriptNativeVariable
{
public:
	idSWFScriptNativeVariable_Nested() : pThis( NULL ) { }
	idSWFScriptNativeVariable_Nested* Bind( T* p )
	{
		pThis = p;
		return this;
	}
	virtual void Set( class idSWFScriptObject* object, const idSWFScriptVar& value ) = 0;
	virtual idSWFScriptVar Get( class idSWFScriptObject* object ) = 0;
protected:
	T* pThis;
};

#define SWF_NATIVE_VAR_DECLARE_NESTED( x, y ) \
	class idSWFScriptNativeVar_##x : public idSWFScriptNativeVariable_Nested<y> {	\
	public:																			\
		void Set( class idSWFScriptObject * object, const idSWFScriptVar & value );	\
		idSWFScriptVar Get( class idSWFScriptObject * object );						\
	} swfScriptVar_##x;

#define SWF_NATIVE_VAR_DECLARE_NESTED_READONLY( x, y, z )							\
	class idSWFScriptNativeVar_##x : public idSWFScriptNativeVariable_Nested<y> {	\
	public:																			\
		bool IsReadOnly() { return true; }											\
		void Set( class idSWFScriptObject * object, const idSWFScriptVar & value ) { assert( false ); } \
		idSWFScriptVar Get( class idSWFScriptObject * object ) { return pThis->z; }	\
	} swfScriptVar_##x;

/*
========================
An object in an action script is a collection of variables. functions are also variables.
========================
*/
class idSWFScriptObject
{
public:
	idSWFScriptObject();
	virtual					~idSWFScriptObject();

	static idSWFScriptObject* 	Alloc();
	void					AddRef();
	void					Release();
	void					SetNoAutoDelete( bool b )
	{
		noAutoDelete = b;
	}

	void					Clear();

	void					MakeArray();

	void					SetSprite( idSWFSpriteInstance* s )
	{
		objectType = SWF_OBJECT_SPRITE;
		data.sprite = s;
	}
	idSWFSpriteInstance* 	GetSprite()
	{
		return ( objectType == SWF_OBJECT_SPRITE ) ? data.sprite : NULL;
	}

	void					SetText( idSWFTextInstance* t )
	{
		objectType = SWF_OBJECT_TEXT;
		data.text = t;
	}
	idSWFTextInstance* 		GetText()
	{
		return ( objectType == SWF_OBJECT_TEXT ) ? data.text : NULL;
	}

	// Also accessible via __proto__ property
	idSWFScriptObject* 		GetPrototype()
	{
		return prototype;
	}
	void					SetPrototype( idSWFScriptObject* _prototype )
	{
		assert( prototype == NULL );
		prototype = _prototype;
		prototype->AddRef();
	}
	idSWFScriptVar			Get( int index );
	idSWFScriptVar			Get( const char* name );
	idSWFSpriteInstance* 	GetSprite( int index );
	idSWFSpriteInstance* 	GetSprite( const char* name );
	idSWFScriptObject* 		GetObject( int index );
	idSWFScriptObject* 		GetObject( const char* name );
	idSWFTextInstance* 		GetText( int index );
	idSWFTextInstance* 		GetText( const char* name );
	void					Set( int index, const idSWFScriptVar& value );
	void					Set( const char* name, const idSWFScriptVar& value );
	void					SetNative( const char* name, idSWFScriptNativeVariable* native );
	bool					HasProperty( const char* name );
	bool					HasValidProperty( const char* name );
	idSWFScriptVar			DefaultValue( bool stringHint );

	// This is to implement for-in (fixme: respect DONTENUM flag)
	int						NumVariables()
	{
		return variables.Num();
	}
	const char* 			EnumVariable( int i )
	{
		return variables[i].name;
	}

	idSWFScriptVar			GetNestedVar( const char* arg1, const char* arg2 = NULL, const char* arg3 = NULL, const char* arg4 = NULL, const char* arg5 = NULL, const char* arg6 = NULL );
	idSWFScriptObject* 		GetNestedObj( const char* arg1, const char* arg2 = NULL, const char* arg3 = NULL, const char* arg4 = NULL, const char* arg5 = NULL, const char* arg6 = NULL );
	idSWFSpriteInstance* 	GetNestedSprite( const char* arg1, const char* arg2 = NULL, const char* arg3 = NULL, const char* arg4 = NULL, const char* arg5 = NULL, const char* arg6 = NULL );
	idSWFTextInstance* 		GetNestedText( const char* arg1, const char* arg2 = NULL, const char* arg3 = NULL, const char* arg4 = NULL, const char* arg5 = NULL, const char* arg6 = NULL );

	void					PrintToConsole() const;

private:
	std::atomic<int> refCount;
	bool noAutoDelete;

	enum swfNamedVarFlags_t
	{
		SWF_VAR_FLAG_NONE = 0,
		SWF_VAR_FLAG_READONLY = BIT( 1 ),
		SWF_VAR_FLAG_DONTENUM = BIT( 2 )
	};
	struct swfNamedVar_t
	{
		swfNamedVar_t() : native( NULL ) { }
		~swfNamedVar_t();
		swfNamedVar_t& operator=( const swfNamedVar_t& other );

		int							index;
		int							hashNext;
		idStr						name;
		idSWFScriptVar				value;
		idSWFScriptNativeVariable* 	native;
		int							flags;
	};
	idList< swfNamedVar_t, TAG_SWF >	variables;

	static const int VARIABLE_HASH_BUCKETS = 16;
	int	variablesHash[VARIABLE_HASH_BUCKETS];

	idSWFScriptObject* 		prototype;

	enum swfObjectType_t
	{
		SWF_OBJECT_OBJECT,
		SWF_OBJECT_ARRAY,
		SWF_OBJECT_SPRITE,
		SWF_OBJECT_TEXT
	} objectType;

	union swfObjectData_t
	{
		idSWFSpriteInstance* 	sprite;			// only valid if objectType == SWF_OBJECT_SPRITE
		idSWFTextInstance* 		text;			// only valid if objectType == SWF_OBJECT_TEXT
	} data;

	swfNamedVar_t* 	GetVariable( int index, bool create );
	swfNamedVar_t* 	GetVariable( const char* name, bool create );
};