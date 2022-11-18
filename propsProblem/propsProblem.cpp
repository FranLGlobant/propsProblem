// El deseo es tener una forma común de almacenar y consultar propiedades de objetos de manera genérica,
// al menor costo en términos de performance y eficiencia, y de la manera más segura posible; pero a su vez
// dando flexibilidad para su reutilización.
// 
// Teniendo en cuenta esos parámetros, hay herramientas que debemos descartar:
// - dynamuc_cast: es muy costoso porque necesita de RTTI (RunTime Type Info), que agrega overhead tanto a
//                 nivel compilación y binario como en runtime.
// - std::any: es muy costoso dado que se apoya en dynamic_cast
// - std::variant: el problema es que necesita en tiempo de compilación conocer todos los tipos de datos
//                 posibles que debería poder almacenar, limitando así su mantenimiento y escalabilidad
// - void*: atenta fuertemente contra la seguridad de los tipos de dato y los casteos.
//
// Como pista, este problema lo pude resolver utilizando:
// - Clases
// - Templates
// - if constexpr
// - Type traits
// - std::optional
// - std::unique_ptr
// - std::reference_wrapper
//
// Eso no significa que no se pueda resolver de otras formas! ;)
// 

#include <optional>
#include <functional>
#include <type_traits>
#include <iostream>



enum class EntityType
{
    Invalid,
    Building,
    Unit,
    Resource
};

enum class BuildingType
{
    Invalid,
    TownCenter,
    Barracks,
    ArcheryRange,
    Stable,
    Monastery,
    SiegeWorkshop
};

enum class UnitType
{
    Invalid,
    Worker,
    Archer,
    Swordman,
    Horseman,
    Monk,
    Mangonel
};

enum class ResourceType
{
    Invalid,
    Food,
    Wood,
    Gold
};

enum class BasePropertyEnum
{
    Invalid
};

enum class CommonPropertyId
{
    Invalid,
    HealthPoints,
    Bounds
};

enum class BuildingPropertyId
{
    Invalid,
    BuildingType,
    Bounds
};

enum class UnitPropertyId
{
    Invalid,
    MovementSpeed
};

class BaseProperty
{
public:
    BaseProperty() = default;
    ~BaseProperty() = default;

    template<class T>
    std::optional<T> get() {
      return std::nullopt;
    }

};

template<class T>
class Property : public BaseProperty
{
public:
    Property(T obj) : BaseProperty(), mObj(obj)
    {

    }

    void set(T obj)
    {
        mObj = obj;
    }

    template <class T>
    std::optional<T> get()
    {
        return mObj;
    }


private:
    T mObj;
};





class Bounds
{
public:
    Bounds(int x, int y, int width, int height) :
        mX(x),
        mY(y),
        mWidth(width),
        mHeight(height)
    {
    }

    int x() const
    {
        return mX;
    }

    int y() const
    {
        return mY;
    }

    int width() const
    {
        return mWidth;
    }

    int height() const
    {
        return mHeight;
    }

    bool contains(int x, int y) const
    {
        return x >= this->x() && x <= this->x() + width() && y >= this->y() && y <= this->y() + height();
    }

private:
    int mX;
    int mY;
    int mWidth;
    int mHeight;
};


class Properties
{
public:

    template <class IdType, class ValueType>
    void set(IdType const& id, ValueType&& value)
    {
        std::shared_ptr<Property <ValueType>> newprop = (std::make_shared<Property<ValueType>>(Property<ValueType>(value)));

        if constexpr (std::is_enum<IdType>::value)
        {
            size_t hash = std::hash<size_t>  {}(static_cast<std::size_t>(id));
            mProperties.insert({ hash, newprop });
        }
        else if constexpr(std::is_trivial<IdType>::value)
        {
            size_t hash = std::hash<IdType>{}(id);
            mProperties.insert({ hash, newprop });
        }
        else
        {
            mProperties.insert({ std::hash<size_t>{}(sizeof(IdType), newprop) });
        }


    }

    template <class IdType, class ValueType>
    std::optional<ValueType const> get(IdType const& id) const
    {
        if constexpr (std::is_enum<IdType>::value)
        {
            auto res = mProperties.find(std::hash<size_t> {} (static_cast<std::size_t>(id)));
            if (res != mProperties.end())
            {
                return ( * (res->second.get())).get<ValueType>().value();
            }
            return std::nullopt;
        }
        else if constexpr (std::is_trivial<IdType>::value)
        {
            auto res = mProperties.find(std::hash<size_t> {} (std::hash(id)));
            if (res != mProperties.end())
            {
                return (*(res->second.get())).get<ValueType>().value();
            }
            return std::nullopt;
        }
        else
        {
            auto const idSize = (sizeof(IdType));
            auto res = mProperties.find(std::hash<size_t> {} (idSize));
            if (res != mProperties.end())
            {
                return (*(res->second.get())).get<ValueType>().value();
            }
            return std::nullopt;
        }
    }

    template <class IdType, class ValueType>
    ValueType const& get(IdType const& id, ValueType const& fallback) const
    {
        auto value = get<IdType, ValueType>(id);
        if (!value.has_value())
        {
            return fallback;
        }
        return value.value();
    }

private:
    std::unordered_map<size_t, std::shared_ptr<BaseProperty>> mProperties = {};
};

class Entity
{
public:

    Entity(EntityType type) : mType(type)

    {

    }

    Properties const& properties() const
    {
        return mProps;
    }
    Properties& properties() 
    {
        return mProps;
    }

private:
    EntityType mType;
    Properties mProps;
};


int main()
{
    Entity e(EntityType::Building);
    e.properties().set(BuildingPropertyId::BuildingType, BuildingType::TownCenter);
    e.properties().set(BuildingPropertyId::Bounds, Bounds{ 0, 0, 2, 2 });

    //auto buildingTypeA = e.properties().get<BuildingType>(BuildingPropertyId::BuildingType);
    auto buildingTypeB = e.properties().get(BuildingPropertyId::BuildingType, BuildingType::Invalid);
    auto buildingTypeC = e.properties().get(BuildingPropertyId::BuildingType, 0); // throw a runtime assert because Id::BuildingType is not of type integer (debug only; in release builds it should just return the specificed fallback value)
    auto extraScenario = e.properties().get(BuildingPropertyId::Invalid, 0); // we don't have that property; assert in debug, return fallback in release

    return 0;
}
