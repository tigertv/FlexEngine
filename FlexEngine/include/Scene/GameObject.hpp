#pragma once

#include "Audio/AudioCue.hpp"
#include "Callbacks/InputCallbacks.hpp"
#include "Graphics/RendererTypes.hpp"
#include "Graphics/VertexBufferData.hpp" // For VertexBufferDataCreateInfo
#include "Spring.hpp"
#include "Transform.hpp"

class btCollisionShape;

namespace flex
{
	class BaseScene;
	class MeshComponent;
	class BezierCurveList;
	class TerminalCamera;
	struct AST;
	struct Tokenizer;

	class GameObject
	{
	public:
		GameObject(const std::string& name, GameObjectType type);
		virtual ~GameObject();

		// Returns a new game object which is a direct copy of this object, parented to parent
		// If parent == nullptr then new object will have same parent as this object
		virtual GameObject* CopySelfAndAddToScene(GameObject* parent, bool bCopyChildren);

		static GameObject* CreateObjectFromJSON(const JSONObject& obj, BaseScene* scene, MaterialID overriddenMatID = InvalidMaterialID);

		virtual void Initialize();
		virtual void PostInitialize();
		virtual void Destroy();
		virtual void Update();

		virtual void DrawImGuiObjects();
		// Returns true if this object was deleted or duplicated
		virtual bool DoImGuiContextMenu(bool bActive);
		virtual bool DoDuplicateGameObjectButton(const char* buttonName);

		virtual Transform* GetTransform();
		virtual const Transform* GetTransform() const;

		virtual void OnTransformChanged();

		virtual bool AllowInteractionWith(GameObject* gameObject);
		virtual void SetInteractingWith(GameObject* gameObject);
		bool IsBeingInteractedWith() const;

		GameObject* GetObjectInteractingWith();

		JSONObject Serialize(const BaseScene* scene) const;
		void ParseJSON(const JSONObject& obj, BaseScene* scene, MaterialID overriddenMatID = InvalidMaterialID);

		void RemoveRigidBody();

		void SetParent(GameObject* parent);
		GameObject* GetParent();
		void DetachFromParent();

		// Returns a list of objects, starting with the root, going up to this object
		std::vector<GameObject*> GetParentChain();

		// Walks up the tree to the highest parent
		GameObject* GetRootParent();

		GameObject* AddChild(GameObject* child);
		bool RemoveChild(GameObject* child);
		const std::vector<GameObject*>& GetChildren() const;

		bool HasChild(GameObject* child, bool bCheckChildrensChildren);

		void UpdateSiblingIndices(i32 myIndex);
		i32 GetSiblingIndex() const;

		// Returns all objects who share our parent
		std::vector<GameObject*> GetAllSiblings();
		// Returns all objects who share our parent and have a larger sibling index
		std::vector<GameObject*> GetEarlierSiblings();
		// Returns all objects who share our parent and have a smaller sibling index
		std::vector<GameObject*> GetLaterSiblings();

		void AddTag(const std::string& tag);
		bool HasTag(const std::string& tag);
		std::vector<std::string> GetTags() const;

		RenderID GetRenderID() const;
		void SetRenderID(RenderID renderID);

		std::string GetName() const;
		void SetName(const std::string& newName);

		bool IsSerializable() const;
		void SetSerializable(bool bSerializable);

		bool IsStatic() const;
		void SetStatic(bool bStatic);

		bool IsVisible() const;
		virtual void SetVisible(bool bVisible, bool bEffectChildren = true);

		// If bIncludingChildren is true, true will be returned if this or any children are visible in scene explorer
		bool IsVisibleInSceneExplorer(bool bIncludingChildren = false) const;
		void SetVisibleInSceneExplorer(bool bVisibleInSceneExplorer);

		bool HasUniformScale() const;
		void SetUseUniformScale(bool bUseUniformScale, bool bEnforceImmediately);

		btCollisionShape* SetCollisionShape(btCollisionShape* collisionShape);
		btCollisionShape* GetCollisionShape() const;

		RigidBody* SetRigidBody(RigidBody* rigidBody);
		RigidBody* GetRigidBody() const;

		MeshComponent* GetMeshComponent();
		MeshComponent* SetMeshComponent(MeshComponent* meshComponent);

		bool CastsShadow() const;
		void SetCastsShadow(bool bCastsShadow);

		// Called when another object has begun to overlap
		void OnOverlapBegin(GameObject* other);
		// Called when another object is no longer overlapping
		void OnOverlapEnd(GameObject* other);

		GameObjectType GetType() const;

		void AddSelfAndChildrenToVec(std::vector<GameObject*>& vec);
		void RemoveSelfAndChildrenToVec(std::vector<GameObject*>& vec);

		// Filled if this object is a trigger
		std::vector<GameObject*> overlappingObjects;

	protected:
		friend BaseScene;

		static const char* s_DefaultNewGameObjectName;

		virtual void ParseUniqueFields(const JSONObject& parentObject, BaseScene* scene, MaterialID matID);
		virtual void SerializeUniqueFields(JSONObject& parentObject) const;

		void CopyGenericFields(GameObject* newGameObject, GameObject* parent, bool bCopyChildren);

		// Returns a string containing our name with a "_xx" post-fix where xx is the next highest index or 00

		std::string m_Name;

		std::vector<std::string> m_Tags;

		Transform m_Transform;
		RenderID m_RenderID = InvalidRenderID;

		GameObjectType m_Type = GameObjectType::_NONE;

		/*
		* If true, this object will be written out to file
		* NOTE: If false, children will also not be serialized
		*/
		bool m_bSerializable = true;

		/*
		* Whether or not this object should be rendered
		* NOTE: Does *not* effect childrens' visibility
		*/
		bool m_bVisible = true;

		/*
		* Whether or not this object should be shown in the scene explorer UI
		* NOTE: Children are also hidden when this if false!
		*/
		bool m_bVisibleInSceneExplorer = true;

		/*
		* True if and only if this object will never move
		* If true, this object will be rendered to reflection probes
		*/
		bool m_bStatic = false;

		/*
		* If true this object will not collide with other game objects
		* Overlapping objects will cause OnOverlapBegin/End to be called
		*/
		bool m_bTrigger = false;

		/*
		* True if this object can currently be interacted with (can be based on
		* player proximity, among other things)
		*/
		bool m_bInteractable = false;

		bool m_bLoadedFromPrefab = false;

		bool m_bBeingInteractedWith = false;

		bool m_bCastsShadow = true;

		// Editor only
		bool m_bUniformScale = false;

		std::string m_PrefabName;

		/*
		* Will point at the player we're interacting with, or the object if we're the player
		*/
		GameObject* m_ObjectInteractingWith = nullptr;

		i32 m_SiblingIndex = 0;

		btCollisionShape* m_CollisionShape = nullptr;
		RigidBody* m_RigidBody = nullptr;

		GameObject* m_Parent = nullptr;
		std::vector<GameObject*> m_Children;

		MeshComponent* m_MeshComponent = nullptr;

		static AudioSourceID s_BunkSound;
		static AudioCue s_SqueakySounds;

	};

	// Child classes

	class DirectionalLight : public GameObject
	{
	public:
		DirectionalLight();
		explicit DirectionalLight(const std::string& name);

		virtual void Initialize() override;
		virtual void Destroy() override;
		virtual void DrawImGuiObjects() override;
		virtual void SetVisible(bool bVisible, bool bEffectChildren /* = true */) override;
		virtual void OnTransformChanged() override;

		bool operator==(const DirectionalLight& other);

		void SetPos(const glm::vec3& newPos);
		glm::vec3 GetPos() const;
		void SetRot(const glm::quat& newRot);
		glm::quat GetRot() const;

		DirLightData data;

		// DEBUG:
		glm::vec3 pos = VEC3_ZERO;

	protected:
		virtual void ParseUniqueFields(const JSONObject& parentObject, BaseScene* scene, MaterialID matID) override;
		virtual void SerializeUniqueFields(JSONObject& parentObject) const override;
	};

	class PointLight : public GameObject
	{
	public:
		explicit PointLight(BaseScene* scene);
		explicit PointLight(const std::string& name);

		virtual void Initialize() override;
		virtual void Destroy() override;
		virtual void DrawImGuiObjects() override;
		virtual void SetVisible(bool bVisible, bool bEffectChildren /* = true */) override;
		virtual void OnTransformChanged() override;

		bool operator==(const PointLight& other);

		void SetPos(const glm::vec3& pos);
		glm::vec3 GetPos() const;

		PointLightData data;
		PointLightID ID = InvalidPointLightID;

	protected:
		virtual void ParseUniqueFields(const JSONObject& parentObject, BaseScene* scene, MaterialID matID) override;
		virtual void SerializeUniqueFields(JSONObject& parentObject) const override;
	};

	class Valve : public GameObject
	{
	public:
		explicit Valve(const std::string& name);

		virtual GameObject* CopySelfAndAddToScene(GameObject* parent, bool bCopyChildren) override;

		virtual void PostInitialize() override;
		virtual void Update() override;

		// Serialized fields
		real minRotation = 0.0f;
		real maxRotation = 0.0f;

		// Non-serialized fields
		// Multiplied with value retrieved from input manager
		real rotationSpeedScale = 1.0f;

		// 1 = never slow down, 0 = slow down immediately
		real invSlowDownRate = 0.85f;

		real rotationSpeed = 0.0f;
		real pRotationSpeed = 0.0f;

		real rotation = 0.0f;
		real pRotation = 0.0f;

	protected:
		virtual void ParseUniqueFields(const JSONObject& parentObject, BaseScene* scene, MaterialID matID) override;
		virtual void SerializeUniqueFields(JSONObject& parentObject) const override;

	};

	class RisingBlock : public GameObject
	{
	public:
		explicit RisingBlock(const std::string& name);

		virtual GameObject* CopySelfAndAddToScene(GameObject* parent, bool bCopyChildren) override;

		virtual void Initialize() override;
		virtual void PostInitialize() override;
		virtual void Update() override;

		// Serialized fields
		Valve* valve = nullptr; // (object name is serialized)
		glm::vec3 moveAxis;

		// If true this block will "fall" to its minimum
		// value when a player is not interacting with it
		bool bAffectedByGravity = false;

		// Non-serialized fields
		glm::vec3 startingPos;

		real pdDistBlockMoved = 0.0f;

	protected:
		virtual void ParseUniqueFields(const JSONObject& parentObject, BaseScene* scene, MaterialID matID) override;
		virtual void SerializeUniqueFields(JSONObject& parentObject) const override;

	};

	class GlassPane : public GameObject
	{
	public:
		explicit GlassPane(const std::string& name);

		virtual GameObject* CopySelfAndAddToScene(GameObject* parent, bool bCopyChildren) override;

		bool bBroken = false;

	protected:
		virtual void ParseUniqueFields(const JSONObject& parentObject, BaseScene* scene, MaterialID matID) override;
		virtual void SerializeUniqueFields(JSONObject& parentObject) const override;

	};

	class ReflectionProbe : public GameObject
	{
	public:
		explicit ReflectionProbe(const std::string& name);

		virtual GameObject* CopySelfAndAddToScene(GameObject* parent, bool bCopyChildren) override;

		virtual void PostInitialize() override;

		MaterialID captureMatID = 0;

	protected:
		virtual void ParseUniqueFields(const JSONObject& parentObject, BaseScene* scene, MaterialID matID) override;
		virtual void SerializeUniqueFields(JSONObject& parentObject) const override;

	};

	class Skybox : public GameObject
	{
	public:
		explicit Skybox(const std::string& name);

		virtual GameObject* CopySelfAndAddToScene(GameObject* parent, bool bCopyChildren) override;

	protected:
		virtual void ParseUniqueFields(const JSONObject& parentObject, BaseScene* scene, MaterialID matID) override;
		virtual void SerializeUniqueFields(JSONObject& parentObject) const override;

	};

	class EngineCart;

	class Cart : public GameObject
	{
	public:
		Cart(CartID cartID, GameObjectType type = GameObjectType::CART);
		Cart(CartID cartID, const std::string& name, GameObjectType type = GameObjectType::CART, const char* meshName = emptyCartMeshName);

		virtual GameObject* CopySelfAndAddToScene(GameObject* parent, bool bCopyChildren) override;

		virtual void DrawImGuiObjects() override;
		virtual real GetDrivePower() const;

		void OnTrackMount(TrackID trackID, real newDistAlongTrack);
		void OnTrackDismount();

		void SetItemHolding(GameObject* obj);
		void RemoveItemHolding();

		// Advances along track, rotates to face correct direction
		void AdvanceAlongTrack(real dT);

		// Returns velocity
		real UpdatePosition();

		CartID cartID = InvalidCartID;

		TrackID currentTrackID = InvalidTrackID;
		real distAlongTrack = -1.0f;
		real velocityT = 1.0f;

		real distToRearNeighbor = -1.0f;

		// Non-serialized fields
		real attachThreshold = 1.5f;

		Spring<real> m_TSpringToCartAhead;

		CartChainID chainID = InvalidCartChainID;

		static const char* emptyCartMeshName;

	protected:
		virtual void ParseUniqueFields(const JSONObject& parentObject, BaseScene* scene, MaterialID matID) override;
		virtual void SerializeUniqueFields(JSONObject& parentObject) const override;

	};

	class EngineCart : public Cart
	{
	public:
		explicit EngineCart(CartID cartID);
		EngineCart(CartID cartID, const std::string& name);

		virtual GameObject* CopySelfAndAddToScene(GameObject* parent, bool bCopyChildren) override;

		virtual void Update() override;
		virtual void DrawImGuiObjects() override;
		virtual real GetDrivePower() const override;


		real moveDirection = 1.0f; // -1.0f or 1.0f
		real powerRemaining = 1.0f;

		real powerDrainMultiplier = 0.1f;
		real speed = 0.1f;

		static const char* engineMeshName;

	protected:
		virtual void ParseUniqueFields(const JSONObject& parentObject, BaseScene* scene, MaterialID matID) override;
		virtual void SerializeUniqueFields(JSONObject& parentObject) const override;

	};

	class MobileLiquidBox : public GameObject
	{
	public:
		MobileLiquidBox();
		explicit MobileLiquidBox(const std::string& name);

		virtual GameObject* CopySelfAndAddToScene(GameObject* parent, bool bCopyChildren) override;

		virtual void DrawImGuiObjects() override;

		bool bInCart = false;
		real liquidAmount = 0.0f;

	protected:
		virtual void ParseUniqueFields(const JSONObject& parentObject, BaseScene* scene, MaterialID matID) override;
		virtual void SerializeUniqueFields(JSONObject& parentObject) const override;

	};

	class GerstnerWave : public GameObject
	{
	public:
		explicit GerstnerWave(const std::string& name);

		virtual void Update() override;
		void AddWave();
		void RemoveWave(i32 index);

		virtual GameObject* CopySelfAndAddToScene(GameObject* parent, bool bCopyChildren) override;

		virtual void DrawImGuiObjects() override;

	protected:
		virtual void ParseUniqueFields(const JSONObject& parentObject, BaseScene* scene, MaterialID matID) override;
		virtual void SerializeUniqueFields(JSONObject& parentObject) const override;

		void UpdateDependentVariables(i32 waveIndex);

		i32 vertSideCount = 100;
		real size = 30.0f;
		VertexBufferDataCreateInfo bufferInfo;

		struct WaveInfo
		{
			bool enabled = true;
			real a = 0.35f;
			real waveDirTheta = 0.5f;
			real waveLen = 5.0f;

			// Non-serialized, calculated from fields above
			real waveDirCos;
			real waveDirSin;
			real moveSpeed = -1.0f;
			real waveVecMag = -1.0f;
			real accumOffset = 0.0f;
		};

		std::vector<WaveInfo> waves;

		GameObject* bobber = nullptr;
		Spring<real> bobberTarget;

	};

	class Blocks : public GameObject
	{
	public:
		explicit Blocks(const std::string& name);

		virtual void Update() override;

	protected:

	};

	class Terminal : public GameObject
	{
	public:
		Terminal();
		explicit Terminal(const std::string& name);

		virtual void Initialize() override;
		virtual void Destroy() override;
		virtual void Update() override;
		virtual void DrawImGuiObjects() override;

		virtual GameObject* CopySelfAndAddToScene(GameObject* parent, bool bCopyChildren) override;

		virtual bool AllowInteractionWith(GameObject* gameObject) override;

		void SetCamera(TerminalCamera* camera);

	protected:
		void TypeChar(char c);
		void DeleteChar(bool bDeleteUpToNextBreak = false); // (backspace)
		void DeleteCharInFront(bool bDeleteUpToNextBreak = false); // (delete)
		void Clear();

		void MoveCursorToStart();
		void MoveCursorToStartOfLine();
		void MoveCursorToEnd();
		void MoveCursorToEndOfLine();
		void MoveCursorLeft(bool bSkipToNextBreak = false);
		void MoveCursorRight(bool bSkipToNextBreak = false);
		void MoveCursorUp();
		void MoveCursorDown();

		void ClampCursorX();

		virtual void ParseUniqueFields(const JSONObject& parentObject, BaseScene* scene, MaterialID matID) override;
		virtual void SerializeUniqueFields(JSONObject& parentObject) const override;

	private:
		friend TerminalCamera;

		EventReply OnKeyEvent(KeyCode keyCode, KeyAction action, i32 modifiers);
		KeyEventCallback<Terminal> m_KeyEventCallback;

		bool SkipOverChar(char c);
		i32 GetIdxOfNextBreak(i32 y, i32 startX);
		i32 GetIdxOfPrevBreak(i32 y, i32 startX);

		void ParseCode();
		void EvaluateCode();

		AST* ast = nullptr;
		Tokenizer* tokenizer = nullptr;

		std::vector<std::string> lines;

		real m_LineHeight = 9.0f;
		real m_LetterScale = 0.23f;

		glm::vec2i cursor;
		// Keeps track of the cursor x to be able to position the cursor correctly
		// after moving from a long line, over a short line, onto a longer line again
		i32 cursorMaxX = 0;

		// Non-serialized fields:
		TerminalCamera* m_Camera = nullptr;
		const i32 m_CharsWide = 45;

		const sec m_CursorBlinkRate = 0.6f;
		sec m_CursorBlinkTimer = 0.0f;

	};

	class ParticleSystem : public GameObject
	{
	public:
		explicit ParticleSystem(const std::string& name);

		virtual GameObject* CopySelfAndAddToScene(GameObject* parent, bool bCopyChildren) override;

		virtual void Update() override;
		virtual void Destroy() override;

		virtual void DrawImGuiObjects() override;

		virtual void OnTransformChanged() override;

		virtual void ParseUniqueFields(const JSONObject& parentObject, BaseScene* scene, MaterialID matID) override;
		virtual void SerializeUniqueFields(JSONObject& parentObject) const override;

		glm::mat4 model;
		real scale;
		ParticleSimData data;
		bool bEnabled;
		MaterialID simMaterialID = InvalidMaterialID;
		MaterialID renderingMaterialID = InvalidMaterialID;
		ParticleSystemID ID = InvalidParticleSystemID;

	private:
		void UpdateModelMatrix();

	};

} // namespace flex
