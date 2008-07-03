/* This file is part of KDevelop
    Copyright 2006 Roberto Raggi <roberto@kdevelop.org>
    Copyright 2006 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef TYPESYSTEM_H
#define TYPESYSTEM_H

#include <QtCore/QSet>
#include <QtCore/QList>

#include "typepointer.h"
#include "language/duchain/identifier.h"

namespace KDevelop
{

class AbstractType;
class IntegralType;
class PointerType;
class ReferenceType;
class FunctionType;
class StructureType;
class ArrayType;

class AbstractTypeData;
class IntegralTypeData;
class PointerTypeData;
class ReferenceTypeData;
class FunctionTypeData;
class StructureTypeData;
class ArrayTypeData;
class DelayedTypeData;

class IndexedString;
class IndexedType;

#define TYPE_DECLARE_DATA(Class) \
    inline Class##Data* d_func_dynamic() { makeDynamic(); return reinterpret_cast<Class##Data *>(d_ptr); } \
    inline const Class##Data* d_func() const { return reinterpret_cast<const Class##Data *>(d_ptr); }

#define TYPE_D(Class) const Class##Data * const d = d_func()
#define TYPE_D_DYNAMIC(Class) Class##Data * const d = d_func_dynamic()

class KDEVPLATFORMLANGUAGE_EXPORT AbstractTypeFactory {
  public:
  virtual AbstractType* create(AbstractTypeData* /*data*/) const = 0;
  virtual ~AbstractTypeFactory() {
  }
};

template<class T>
class KDEVPLATFORMLANGUAGE_EXPORT TypeFactory : public AbstractTypeFactory {
  public:
  AbstractType* create(AbstractTypeData* data) const {
    return new T(*static_cast<typename T::Data*>(data));
  }
};

class KDEVPLATFORMLANGUAGE_EXPORT TypeSystem {
  public:
    template<class T>
    void registerTypeClass() {
      Q_ASSERT(T::Identity < 64);
      if(m_factories.size() <= T::Identity)
        m_factories.resize(T::Identity+1);
      
      Q_ASSERT(!m_factories[T::Identity]);
      m_factories[T::Identity] = new TypeFactory<T>();
    }
    
    template<class T>
    void unregisterTypeClass() {
      Q_ASSERT(m_factories.size() > T::Identity);
      Q_ASSERT(m_factories[T::Identity]);
      delete m_factories[T::Identity];
      m_factories[T::Identity] = 0;
    }
    
    static TypeSystem& self();
  private:
    QVector<AbstractTypeFactory*> m_factories;
};

template<class T>
struct KDEVPLATFORMLANGUAGE_EXPORT TypeSystemRegistrator {
  TypeSystemRegistrator() {
    TypeSystem::self().registerTypeClass<T>();
  }
  ~TypeSystemRegistrator() {
    TypeSystem::self().unregisterTypeClass<T>();
  }
};

///You must add this into your source-files for every AbstractType based class
///For this to work, the class must have an "Identity" enumerator, that is unique among all types, and should be a very low value.
///The highest allowed identity is 63, so currently we're limited to having 64 different type classes.
#define REGISTER_TYPE(Class) TypeSystemRegistrator<Class> register ## Class

class KDEVPLATFORMLANGUAGE_EXPORT TypeVisitor
{
public:
  virtual ~TypeVisitor ();

  virtual bool preVisit (const AbstractType *) = 0;
  virtual void postVisit (const AbstractType *) = 0;

  ///Return whether sub-types should be visited(same for the other visit functions)
  virtual bool visit(const AbstractType*) = 0;

  virtual void visit (const IntegralType *) = 0;

  virtual bool visit (const PointerType *) = 0;
  virtual void endVisit (const PointerType *) = 0;

  virtual bool visit (const ReferenceType *) = 0;
  virtual void endVisit (const ReferenceType *) = 0;

  virtual bool visit (const FunctionType *) = 0;
  virtual void endVisit (const FunctionType *) = 0;

  virtual bool visit (const StructureType *) = 0;
  virtual void endVisit (const StructureType *) = 0;

  virtual bool visit (const ArrayType *) = 0;
  virtual void endVisit (const ArrayType *) = 0;
};

class KDEVPLATFORMLANGUAGE_EXPORT SimpleTypeVisitor : public TypeVisitor
{
public:
  ///When using SimpleTypeVisitor, this is the only function you must override to collect all types.
  virtual bool visit(const AbstractType*) = 0;

  virtual bool preVisit (const AbstractType *) ;
  virtual void postVisit (const AbstractType *) ;

  virtual void visit (const IntegralType *) ;

  virtual bool visit (const PointerType *) ;
  virtual void endVisit (const PointerType *) ;

  virtual bool visit (const ReferenceType *) ;
  virtual void endVisit (const ReferenceType *) ;

  virtual bool visit (const FunctionType *) ;
  virtual void endVisit (const FunctionType *) ;

  virtual bool visit (const StructureType *) ;
  virtual void endVisit (const StructureType *) ;

  virtual bool visit (const ArrayType *) ;
  virtual void endVisit (const ArrayType *) ;
};

/**
 * A class that can be used to walk through all types that are references from one type, and exchange them with other types.
 * Examples for such types: Base-classes of a class, function-argument types of a function, etc.
 * */
class KDEVPLATFORMLANGUAGE_EXPORT TypeExchanger {
  public:
    virtual ~TypeExchanger() {
    }

    /**
     * By default should return the given type, and can return another type that the given should be replaced with.
     * Types should allow replacing all their held types using this from within their exchangeTypes function.
     * */
    virtual AbstractType* exchange( const AbstractType* ) = 0;
    /**
     * Should member-types be exchanged?(Like the types of a structure's members) If false, only types involved in the identity will be exchanged.
     * */
    virtual bool exchangeMembers() const = 0;
};

class KDEVPLATFORMLANGUAGE_EXPORT AbstractType : public TypeShared
{
public:
  typedef TypePtr<AbstractType> Ptr;

  AbstractType();
  AbstractType(const AbstractType& rhs);
  AbstractType(AbstractTypeData& dd);
  virtual ~AbstractType ();

  void accept(TypeVisitor *v) const;

  static void acceptType(AbstractType::Ptr type, TypeVisitor *v);

  virtual QString toString() const = 0;

  ///Must always be called before anything in the data pointer is changed!
  ///If it's not called beforehand, the type-repository gets corrupted
  void makeDynamic();
  
  ///Should return whether this type's content equals the given one
  ///Since this is used by the type-repository, it must compare ALL members of the data type.
  virtual bool equals(const AbstractType* rhs) const = 0;

  /**
   * Should create a clone of the source-type, with as much data copied as possible without breaking the du-chain.
   * */
  virtual AbstractType* clone() const = 0;

  /**
   * A hash-value that should have the following properties:
   * - When two types match on equals(), it should be same.
   * - When two types don't match on equals(), it should be different with a high probability.
   * */
  virtual uint hash() const = 0;

  ///This can also be called on zero types, those can then be reconstructed from the zero index
  IndexedType indexed() const;
  
  enum WhichType {
    TypeAbstract  /**< an abstract type */,
    TypeIntegral  /**< an integral */,
    TypePointer   /**< a pointer*/,
    TypeReference /**< a reference */,
    TypeFunction  /**< a function */,
    TypeStructure /**< a structure */,
    TypeArray     /**< an array */,
    TypeDelayed   /**< a delayed type */,
    TypeForward   /**< a foward declaration type */
  };

  virtual WhichType whichType() const;

  enum {
    Identity = 1
  };
  
  /**
   * Should, like accept0, be implemented by all types that hold references to other types.
   * */
  virtual void exchangeTypes( TypeExchanger* exchanger );

  typedef AbstractTypeData Data;
  
protected:
  virtual void accept0 (TypeVisitor *v) const = 0;
  AbstractTypeData* const d_ptr;

//  template <class T>
//  void deregister(T* that) { TypeSystem::self()->deregisterType(that); }

  TYPE_DECLARE_DATA(AbstractType)
};

///Allows accessing the type just by a single index, so the reference can be stored to disk etc.
class KDEVPLATFORMLANGUAGE_EXPORT IndexedType {
  public:
    IndexedType(uint index = 0) : m_index(index) {
    }
    
    ///Returns zero type if this index is invalid
    AbstractType::Ptr type() const;
    
    bool isValid() const {
      return (bool)m_index;
    }
    
    operator bool() const {
      return (bool)m_index;
    }
    
    bool operator==(const IndexedType& rhs) const {
      return m_index == rhs.m_index;
    }
    
    uint hash() const {
      return m_index;
    }
    
    uint index() const {
      return m_index;
    }
    
  private:
    uint m_index;
};

class KDEVPLATFORMLANGUAGE_EXPORT IntegralType: public AbstractType
{
public:
  typedef TypePtr<IntegralType> Ptr;

  IntegralType();
  IntegralType(const IntegralType& rhs);
  IntegralType(const IndexedString& name);
  IntegralType(IntegralTypeData& data);
  ~IntegralType();

  const IndexedString& name() const;

  void setName(const IndexedString& name);

  bool operator == (const IntegralType &other) const;

  bool operator != (const IntegralType &other) const;

  virtual QString toString() const;

  virtual uint hash() const;

  virtual WhichType whichType() const;

  virtual AbstractType* clone() const;

  virtual bool equals(const AbstractType* rhs) const;

  enum {
    Identity = 2
  };
  
  typedef IntegralTypeData Data;
  
protected:
  virtual void accept0 (TypeVisitor *v) const;

  TYPE_DECLARE_DATA(IntegralType)
};

class KDEVPLATFORMLANGUAGE_EXPORT PointerType: public AbstractType
{
public:
  typedef TypePtr<PointerType> Ptr;

  PointerType ();
  PointerType(const PointerType& rhs);
  PointerType(PointerTypeData& data);
  ~PointerType();

  bool operator != (const PointerType &other) const;
  bool operator == (const PointerType &other) const;
  void setBaseType(AbstractType::Ptr type);
  AbstractType::Ptr baseType () const;

  virtual QString toString() const;

  virtual uint hash() const;

  virtual WhichType whichType() const;

  virtual AbstractType* clone() const;

  virtual bool equals(const AbstractType* rhs) const;

  virtual void exchangeTypes( TypeExchanger* exchanger );
  
  enum {
    Identity = 3
  };
  
  typedef PointerTypeData Data;
  
protected:
  virtual void accept0 (TypeVisitor *v) const;

  TYPE_DECLARE_DATA(PointerType)
};

class KDEVPLATFORMLANGUAGE_EXPORT ReferenceType: public AbstractType
{
public:
  typedef TypePtr<ReferenceType> Ptr;

  ReferenceType ();
  ReferenceType (const ReferenceType& rhs);
  ReferenceType(ReferenceTypeData& data);
  ~ReferenceType();
  AbstractType::Ptr baseType () const;

  void setBaseType(AbstractType::Ptr baseType);

  bool operator == (const ReferenceType &other) const;

  bool operator != (const ReferenceType &other) const;

  virtual QString toString() const;

  virtual uint hash() const;

  virtual WhichType whichType() const;

  virtual AbstractType* clone() const;

  virtual bool equals(const AbstractType* rhs) const;

  virtual void exchangeTypes( TypeExchanger* exchanger );
  
  enum {
    Identity = 4
  };
  
  typedef ReferenceTypeData Data;
  
protected:
  virtual void accept0 (TypeVisitor *v) const;

  TYPE_DECLARE_DATA(ReferenceType)
};

class KDEVPLATFORMLANGUAGE_EXPORT FunctionType : public AbstractType
{
public:
  typedef TypePtr<FunctionType> Ptr;

  enum SignaturePart {
    SignatureWhole /**< When this is given to toString(..), a string link "RETURNTYPE (ARGTYPE1, ARGTYPE1, ..)" is returned */,
    SignatureReturn /**< When this is given, only a string that represents the return-type is returned */,
    SignatureArguments /**< When this is given, a string that represents the arguments like "(ARGTYPE1, ARGTYPE1, ..)" is returend */
  };

  FunctionType();
  FunctionType(const FunctionType& rhs);
  FunctionType(FunctionTypeData& data);
  ~FunctionType();

  AbstractType::Ptr returnType () const;

  void setReturnType(AbstractType::Ptr returnType);

  QList<AbstractType::Ptr> arguments () const;

  void addArgument(AbstractType::Ptr argument);
  void removeArgument(AbstractType::Ptr argument);

  bool operator == (const FunctionType &other) const;

  bool operator != (const FunctionType &other) const;

  virtual AbstractType* clone() const;

  virtual bool equals(const AbstractType* rhs) const;

  virtual QString toString() const;

  ///Creates a string that represents the given part of the signature
  virtual QString partToString( SignaturePart sigPart ) const;

  virtual uint hash() const;

  virtual WhichType whichType() const;

  virtual void exchangeTypes( TypeExchanger* exchanger );
  
  enum {
    Identity = 5
  };
  
  typedef FunctionTypeData Data;
  
protected:
  virtual void accept0 (TypeVisitor *v) const;

  TYPE_DECLARE_DATA(FunctionType)
};

class KDEVPLATFORMLANGUAGE_EXPORT StructureType : public AbstractType
{
public:
  StructureType();
  StructureType(const StructureType&);
  ~StructureType();
  StructureType(StructureTypeData& data);
  
  typedef TypePtr<StructureType> Ptr;

  virtual AbstractType* clone() const;

  virtual bool equals(const AbstractType* rhs) const;

  virtual QString toString() const;

  virtual uint hash() const;

  virtual WhichType whichType() const;

  enum {
    Identity = 6
  };
  
  typedef StructureTypeData Data;
  
protected:
  virtual void accept0 (TypeVisitor *v) const;

  TYPE_DECLARE_DATA(StructureType)
};

class KDEVPLATFORMLANGUAGE_EXPORT ArrayType : public AbstractType
{
public:
  typedef TypePtr<ArrayType> Ptr;

  ArrayType();
  ArrayType(const ArrayType&);
  ArrayType(ArrayTypeData& data);

  virtual AbstractType* clone() const;

  virtual bool equals(const AbstractType* rhs) const;

  ~ArrayType();

  int dimension () const;

  void setDimension(int dimension);

  AbstractType::Ptr elementType () const;

  void setElementType(AbstractType::Ptr type);

  bool operator == (const ArrayType &other) const;

  bool operator != (const ArrayType &other) const;

  virtual QString toString() const;

  virtual uint hash() const;

  virtual WhichType whichType() const;

  virtual void exchangeTypes( TypeExchanger* exchanger );
  
  enum {
    Identity = 7
  };
  
  typedef ArrayTypeData Data;
  
protected:
  virtual void accept0 (TypeVisitor *v) const;

  TYPE_DECLARE_DATA(ArrayType)
};

/**
 * Delayed types can be used for any types that cannot be resolved in the moment they are encountered.
 * They can be used for example in template-classes, or to store the names of unresolved types.
 * In a template-class, many types can not be evaluated at the time they are used, because they depend on unknown template-parameters.
 * Delayed types store the way the type would be searched, and can be used to find the type once the template-paremeters have values.
 * */
class KDEVPLATFORMLANGUAGE_EXPORT DelayedType : public KDevelop::AbstractType
{
public:
  typedef TypePtr<DelayedType> Ptr;

  enum Kind {
    Delayed /**< The type should be resolved later. This is the default. */,
    Unresolved /**< The type could not be resolved */
  };

  DelayedType();
  DelayedType(const DelayedType& rhs);
  DelayedType(DelayedTypeData& data);
  virtual ~DelayedType();

  KDevelop::TypeIdentifier identifier() const;
  void setIdentifier(const KDevelop::TypeIdentifier& identifier);

  virtual QString toString() const;

  virtual AbstractType* clone() const;

  virtual bool equals(const AbstractType* rhs) const;

  Kind kind() const;
  void setKind(Kind kind);

  virtual uint hash() const;

  virtual WhichType whichType() const;
  
  enum {
    Identity = 8
  };
  
  typedef DelayedTypeData Data;
  
  protected:
    virtual void accept0 (KDevelop::TypeVisitor *v) const ;
    TYPE_DECLARE_DATA(DelayedType)
};

template <class T>
uint qHash(const TypePtr<T>& type) { return (uint)((size_t)type.data()); }


/**
 * You can use these instead of dynamic_cast, for basic types it has better performance because it checks the whichType() member
*/

template<class To>
inline To fastCast(AbstractType* from) {
  return dynamic_cast<To>(from);
}

template<class To>
inline const To fastCast(const AbstractType* from) {
  return const_cast<const To>(fastCast<To>(const_cast<AbstractType*>(from))); //Hack so we don't need to define the functions twice, once for const, and once for not const
}

template<>
inline ReferenceType* fastCast<ReferenceType*>(AbstractType* from) {
  if(!from || from->whichType() != AbstractType::TypeReference)
    return 0;
  else
    return static_cast<ReferenceType*>(from);
}

template<>
inline PointerType* fastCast<PointerType*>(AbstractType* from) {
  if(!from || from->whichType() != AbstractType::TypePointer)
    return 0;
  else
    return static_cast<PointerType*>(from);
}

template<>
inline IntegralType* fastCast<IntegralType*>(AbstractType* from) {
  if(!from || from->whichType() != AbstractType::TypeIntegral)
    return 0;
  else
    return static_cast<IntegralType*>(from);
}

template<>
inline FunctionType* fastCast<FunctionType*>(AbstractType* from) {
  if(!from || from->whichType() != AbstractType::TypeFunction)
    return 0;
  else
    return static_cast<FunctionType*>(from);
}

template<>
inline StructureType* fastCast<StructureType*>(AbstractType* from) {
  if(!from || from->whichType() != AbstractType::TypeStructure)
    return 0;
  else
    return static_cast<StructureType*>(from);
}

template<>
inline ArrayType* fastCast<ArrayType*>(AbstractType* from) {
  if(!from || from->whichType() != AbstractType::TypeArray)
    return 0;
  else
    return static_cast<ArrayType*>(from);
}

template<>
inline DelayedType* fastCast<DelayedType*>(AbstractType* from) {
  if(!from || from->whichType() != AbstractType::TypeDelayed)
    return 0;
  else
    return static_cast<DelayedType*>(from);
}

}




#endif // TYPESYSTEM_H

// kate: space-indent on; indent-width 2; tab-width 4; replace-tabs on; auto-insert-doxygen on
